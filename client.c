#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define SHARED_MEM_NAME "/shared_memory" // имя разделяемой памяти

#define NUM_FLOWERS 40 // Количество цветов в саду (по условию их 40)

char *ip;
int port;
int server;

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

void Send(int from, int message) {
    char send[] = "00";

    send[0] = (char) from;
    send[1] = (char) message;

    write(server, send, 2);
}

struct shared_mem {
    int garden[NUM_FLOWERS];        // Массив с цветками в саду
};

void first_gardener(sem_t *sem1, struct shared_mem *shared_mem_ptr) {
    srand(time(NULL));

    Send(1, -1);

    while (1) {
        sem_wait(sem1);

        int index = rand() % NUM_FLOWERS;
        printf("Начинает вянуть цветок под номером %d\n", index);
        if (shared_mem_ptr->garden[index] == 1) {
            Send(1, -2);
            continue;
        }
        shared_mem_ptr->garden[index] = 1; // Начинаем поливать цветок
        sleep(rand() % 5 + 1); // Немного ждём
        shared_mem_ptr->garden[index] = 0; // Полили цветок

        Send(1, index);

        sem_post(sem1);
    }
}

void second_gardener(sem_t *sem1, struct shared_mem *shared_mem_ptr) {

    Send(2, -1);

    while (1) {
        sem_wait(sem1);

        int index = rand() % NUM_FLOWERS;
        printf("Начинает вянуть цветок под номером %d\n", index);
        if (shared_mem_ptr->garden[index] == 1) {
            Send(2, -2);
            continue;
        }
        shared_mem_ptr->garden[index] = 1; // Начинаем поливать цветок
        sleep(rand() % 5 + 1); // Немного ждём
        shared_mem_ptr->garden[index] = 0; // Полили цветок

        Send(2, index);

        sem_post(sem1);
    }
}

int main(int argc, char **argv) {

    // Произвольные порты
    ip = "127.0.0.1";
    port = 11111;

    if (argc != 3) {
        printf("ERROR: неверный формат команды запуска");
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    server = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adr = {0};
    adr.sin_family = AF_INET;
    adr.sin_port = htons(port);
    InetPton(AF_INET, ip, &adr.sin_addr);
    Connect(server, (struct sockaddr *) &adr, sizeof adr);

    int first;
    int second;

    sem_t sem;
    sem_t sem2;
    sem_init(&sem, 1, 1);
    sem_init(&sem2, 1, 1);

    int fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, 0666); // 0666
    ftruncate(fd, sizeof(struct shared_mem));
    void *shared_memory = mmap(NULL, sizeof(struct shared_mem),
                               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct shared_mem *shared_mem_ptr = (struct shared_mem *) shared_memory;

    int main_pid = getpid();

    if (main_pid == getpid()) {
        first = fork();
        if (first == 0) {
            first = getpid();
        }
    }

    if (main_pid == getpid()) {
        second = fork();
        if (second == 0) {
            second = getpid();
        }
    }

    if (first == getpid()) {
        first_gardener(&sem, shared_mem_ptr);
    }

    if (second == getpid()) {
        second_gardener(&sem2, shared_mem_ptr);
    }

    if (main_pid == getpid()) {
        wait(&first);
        wait(&second);
        for (int i = 0; i < NUM_FLOWERS; ++i) {
            fflush(NULL);
        }

        sem_destroy(&sem);
        sem_destroy(&sem2);
        shm_unlink(SHARED_MEM_NAME);
    }

    return 0;
}
