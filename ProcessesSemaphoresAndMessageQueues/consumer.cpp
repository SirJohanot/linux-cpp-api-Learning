#include<iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>

#define MAXSIZE 255
struct sembuf semaphoreOperationStructure[2];

typedef struct msg_buf {
    long type;
    char data[MAXSIZE];
} messageBuffer;

int main(int argc, char *argv[]) {
    char *name = argv[0];
    key_t key = ftok(argv[1], 'a');

    int messageQueueId;
    if ((messageQueueId = msgget(key, IPC_EXCL | 0666)) < 0) {
        perror("msgget");
        exit(errno);
    }

    int semaphoreSetId;
    if ((semaphoreSetId = semget(key, 3, IPC_CREAT | 0666)) < 0) {
        perror("semget");
        exit(errno);
    }

    printf("\nChild name = %s, ", name);
    printf("child got messageQueueId = %d, ", messageQueueId);
    printf("got semaphoreSetId = %d\n", semaphoreSetId);

    messageBuffer readBuffer;

    while (true) {
        semaphoreOperationStructure[0].sem_num = 2;
        semaphoreOperationStructure[0].sem_op = -1;
        semaphoreOperationStructure[0].sem_flg = 0;

        semaphoreOperationStructure[1].sem_num = 0;
        semaphoreOperationStructure[1].sem_op = -1;
        semaphoreOperationStructure[1].sem_flg = 0;

        semop(semaphoreSetId, semaphoreOperationStructure, 2);

        if (msgrcv(messageQueueId, &readBuffer, MAXSIZE + 1, 3, IPC_NOWAIT) < 0) {
            perror("msgrcv");
            exit(errno);
        }


        semaphoreOperationStructure[0].sem_num = 0;
        semaphoreOperationStructure[0].sem_op = 1;
        semaphoreOperationStructure[0].sem_flg = 0;
        //win sock
        semaphoreOperationStructure[1].sem_num = 1;
        semaphoreOperationStructure[1].sem_op = 1;
        semaphoreOperationStructure[1].sem_flg = 0;

        semop(semaphoreSetId, semaphoreOperationStructure, 2);

        semaphoreOperationStructure[0].sem_num = 4;
        semaphoreOperationStructure[0].sem_op = 1;
        semop(semaphoreSetId, semaphoreOperationStructure, 1);
        printf("%s got a message from queue: %s\nNumber of got messages: %d\n", name, readBuffer.data,
               semctl(semaphoreSetId, 4, GETVAL));

        sleep(5);
    }

    exit(0);

}