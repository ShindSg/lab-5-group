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
void parse_request_json(const char* json, char* query_out, char* type_out, char* search_type) {
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
    char* t = strstr(json, "\"search_type\"");
    if (t) {
        t = strchr(t, ':');
        if (t) {
            t = strchr(t, '"');
            if (t) {
                t++;
                char* end = strchr(t, '"');
                if (end) {
                    strncpy(search_type, t, end - t);
                    search_type[end - t] = '\0';
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
    char search_type[32] = {0};
    parse_request_json(buffer, query, tree_type, search_type);

    // Пишем логи в stderr, чтобы они сразу без буферизации падали в server.log
    fprintf(stderr, "[Сервер] Поиск: %s (Движок: %s)\n", query, tree_type);
    fflush(stderr);

    Index* current_index = index_avl;
    if (strcmp(tree_type, "rb") == 0) {
        current_index = index_rb;
    } else if (strcmp(tree_type, "btree") == 0) {
        current_index = index_btree;
    }

    if (!current_index) {
        char error_msg[] = "{\"status\":\"error\",\"message\":\"Индекс не загружен на сервере\"}\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
    } else {
        // 1. Вызываем поисковый движок
        if (strcmp(search_type, "normal") == 0) {
            SearchResults* sr = search(current_index, query);
        }
        else {
            SearchResults* sr = fuzzySearch(current_index, query);
        }
        
        // 2. Создаем дубликат дескриптора сокета для работы через файловый поток Си
        int socket_dup = dup(client_socket);
        FILE* socket_stream = fdopen(socket_dup, "w");
        
        if (socket_stream) {
            // Сбрасываем текущий буфер stdout перед подменой
            fflush(stdout);
            
            // Сохраняем оригинальный stdout (консоль/лог)
            int stdout_backup = dup(1);
            
            // Направляем stdout (дескриптор 1) прямиком в сокет клиента
            dup2(socket_dup, 1);
            
            // Твоя функция пишет в stdout, но данные летят по сети в Streamlit!
            printResultsJSON(sr);
            
            // Дописываем перевод строки, чтобы Python-скрипт гарантированно считал пакет до конца
            printf("\n");
            
            // Форсируем отправку всех байт в сеть и возвращаем stdout обратно
            fflush(stdout);
            dup2(stdout_backup, 1);
            
            close(stdout_backup);
            fclose(socket_stream); // Закроет и socket_dup
        } else {
            close(socket_dup);
        }
        
        freeSearchResults(sr);
    }

    close(client_socket);
}

int main() {
    // Включаем немедленный сброс буфера для логов старта
    fprintf(stderr, "[Старт] Загрузка индексов из папки data/test/...\n");
    fflush(stderr);
    
    index_avl   = loadIndex("data/test/idx_avl.txt", TREE_AVL);
    index_rb    = loadIndex("data/test/idx_rb.txt", TREE_RB);
    index_btree = loadIndex("data/test/idx_btree.txt", TREE_BTREE);
    
    fprintf(stderr, "[Старт] Загрузка завершена. Сервер готов к работе.\n");
    fflush(stderr);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
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

    fprintf(stderr, "[Сервер] Слушает TCP порт %d...\n", PORT);
    fflush(stderr);

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