#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int Socket(int domain, int type, int protocol) {
    int res = socket(domain, type, protocol);
    if (res == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    return res;
}

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int res = bind(sockfd, addr, addrlen);
    if (res == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

void Listen(int sockfd, int backlog) {
    int res = listen(sockfd, backlog);
    if (res == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int res = accept(sockfd, addr, addrlen);
    if (res == -1) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    return res;
}

void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int res = connect(sockfd, addr, addrlen);
    if (res == -1) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
}

void InetPton(int af, const char *src, void *dst) {
    int res = inet_pton(af, src, dst);
    if (res == 0) {
        printf("inet_pton failed: src does not contain a character"
               " string representing a valid network address in the specified"
               " address family\n");
        exit(EXIT_FAILURE);
    }
    if (res == -1) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }
}

// Хэндлер для первого садовника
void HandleFirst(int message) {
    if (message == -1) {
        printf("Первый садовник начал свою работу\n");
        return;
    }
    if (message == -2) {
        printf("Первый садовник захотел полить тот же цветок, что и второй, но всё обошлось!\n");
        return;
    }
    printf("Первый садовник полил цветок под номером %d\n", message);
}

// Хэндлер для второго садовника
void HandleSecond(int message) {
    if (message == -1) {
        printf("Второй садовник начал свою работу\n");
        return;
    }
    if (message == -2) {
        printf("Второй садовник захотел полить тот же цветок, что и первый, но всё обошлось!\n");
        return;
    }
    printf("Второй садовник полил цветок под номером %d\n", message);
}

void HandleClient(int from, int message) {
    if (from == 1) { // Обработка запроса для первого садовника
        HandleFirst(message);
        return;
    }

    if (from == 2) { // Обработка запроса для второго садовника
        HandleSecond(message);
        return;
    }
}

int main(int argc, char *argv[]) {
    // Задаём произвольный порт
    int port = 11111;

    if (argc != 2) {
        printf("ERROR: Неверный формат команды запуска");
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[1]);
    if (port < 1 || port > 65536) {
        printf("Порт должен быть в заданном диапазоне [1, 65536]\n");
        exit(EXIT_FAILURE);
    }


    int server = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adr = {0};
    adr.sin_family = AF_INET;
    adr.sin_port = htons(port);
    Bind(server, (struct sockaddr *) &adr, sizeof adr);
    Listen(server, 5);
    socklen_t adrlen = sizeof adr;
    int fd = Accept(server, (struct sockaddr *) &adr, &adrlen);

    while (1) {
        ssize_t nread;
        char buf[2];
        nread = read(fd, buf, 2);

        if (nread == -1) {
            perror("read failed");
            exit(EXIT_FAILURE);
        }
        if (nread == 0) {
            printf("Сервер завершает работу...\n");
            break;
        }

        int from = (int)buf[0];
        int message = (int)buf[1];

        HandleClient(from, message);
    }

    sleep(2);

    close(fd);
    close(server);

    return 0;
}