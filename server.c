#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "index/index.h"
#include "index/search.h"

#define PORT 8080
#define BUFFER_SIZE 65536

// Глобальные указатели на индексы
Index* index_avl   = NULL;
Index* index_rb    = NULL;
Index* index_btree = NULL;

// Простой парсинг JSON запроса от Streamlit
void parse_request_json(const char* json, char* query_out, char* type_out) {
    char* q = strstr(json, "\"query\"");
    if (q) {
        q = strchr(q, ':');
        if (q) {
            q = strchr(q, '"');
            if (q) {
                q++;
                char* end = strchr(q, '"');
                if (end) {
                    strncpy(query_out, q, end - q);
                    query_out[end - q] = '\0';
                }
            }
        }
    }
    char* t = strstr(json, "\"tree_type\"");
    if (t) {
        t = strchr(t, ':');
        if (t) {
            t = strchr(t, '"');
            if (t) {
                t++;
                char* end = strchr(t, '"');
                if (end) {
                    strncpy(type_out, t, end - t);
                    type_out[end - t] = '\0';
                }
            }
        }
    }
}

void handle_client(int client_socket) {
    char buffer[4096] = {0};
    int valread = read(client_socket, buffer, sizeof(buffer) - 1);
    if (valread <= 0) {
        close(client_socket);
        return;
    }

    char query[256] = {0};
    char tree_type[32] = {0};
    parse_request_json(buffer, query, tree_type);

    printf("[Сервер] Поиск: %s (Движок: %s)\n", query, tree_type);

    Index* current_index = index_avl;
    if (strcmp(tree_type, "rb") == 0) {
        current_index = index_rb;
    } else if (strcmp(tree_type, "btree") == 0) {
        current_index = index_btree;
    }

    char* response_json = malloc(BUFFER_SIZE);
    if (!response_json) {
        close(client_socket);
        return;
    }
    memset(response_json, 0, BUFFER_SIZE);

    if (!current_index) {
        snprintf(response_json, BUFFER_SIZE, "{\"status\":\"error\",\"message\":\"Индекс не загружен на сервере\"}\n");
    } else {
        // 1. Вызываем твой родной поиск
        SearchResults* sr = search(current_index, query);
        
        // 2. Перехватываем stdout в буфер response_json через fmemopen
        FILE* mem_stream = fmemopen(response_json, BUFFER_SIZE - 2, "w");
        if (mem_stream) {
            fflush(stdout);
            int stdout_dup = dup(1);      // Сохраняем реальный stdout
            int mem_fd = fileno(mem_stream);
            dup2(mem_fd, 1);              // Подменяем stdout потоком в памяти
            
            printResultsJSON(sr);         // Твоя функция пишет в stdout (то есть в память)
            
            fflush(stdout);
            dup2(stdout_dup, 1);          // Возвращаем stdout на место
            close(stdout_dup);
            fclose(mem_stream);
        }
        freeSearchResults(sr);
    }

    // Дописываем \n, чтобы Python понимал, где конец пакета
    size_t len = strlen(response_json);
    if (len > 0 && response_json[len - 1] != '\n') {
        strcat(response_json, "\n");
    }

    send(client_socket, response_json, strlen(response_json), 0);
    free(response_json);
    close(client_socket);
}

int main() {
    printf("[Старт] Загрузка индексов из папки data/test/...\n");
    index_avl   = loadIndex("data/test/idx_avl.txt", TREE_AVL);
    index_rb    = loadIndex("data/test/idx_rb.txt", TREE_RB);
    index_btree = loadIndex("data/test/idx_btree.txt", TREE_BTREE);
    printf("[Старт] Загрузка завершена. Сервер готов к работе.\n");

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[Сервер] Слушает TCP порт %d...\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(new_socket);
    }

    freeIndex(index_avl);
    freeIndex(index_rb);
    freeIndex(index_btree);
    return 0;
}