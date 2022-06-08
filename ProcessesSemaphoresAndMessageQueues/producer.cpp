#include<iostream>
#include<cstring>
#include <cstdio>
#include <unistd.h>
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


char *getMessage() {
    int len = rand() % 50 + 1;
    char *s = (char *) calloc(len, 1);
    for (int i = 0; i < len; i++) {
        s[i] = (char) (rand() % 26 + 97);
        if (i + 1 == len) {
            s[i] = '\0';
        }
    }
    return s;
}

int main(int argc, char *argv[]) {

    char *name = argv[0];
    key_t key = ftok(argv[1], 'a');
    int messageQueueId;
    int semaphoreSetId;
    messageBuffer sendBuffer;

    if ((messageQueueId = msgget(key, IPC_EXCL | 0666)) < 0) {
        perror("msgget");
        exit(errno);
    }

    key_t sem1_key = 123;
    key_t sem2_key = 123;

    if ((semaphoreSetId = semget(key, 5, IPC_CREAT | 0666)) < 0) {
        perror("semget");
        exit(errno);
    }

    printf("\nChild name = %s, ", name);
    printf("child got messageQueueId = %d, ", messageQueueId);
    printf("got semaphoreSetId = %d\n", semaphoreSetId);

    while (true) {

        semaphoreOperationStructure[0].sem_num = 1;
        semaphoreOperationStructure[0].sem_op = -1;
        semaphoreOperationStructure[0].sem_flg = 0;

        semaphoreOperationStructure[1].sem_num = 0;
        semaphoreOperationStructure[1].sem_op = -1;
        semaphoreOperationStructure[1].sem_flg = 0;

        semop(semaphoreSetId, semaphoreOperationStructure, 2);

        sendBuffer.type = 3;
        strcpy(sendBuffer.data, getMessage());
        if (msgsnd(messageQueueId, &sendBuffer, strlen(sendBuffer.data) + 1, IPC_NOWAIT) < 0) {
            printf("%d, %ld, %s, %ld\n", messageQueueId, sendBuffer.type, sendBuffer.data, strlen(sendBuffer.data) + 1);
            perror("msgsnd");
            exit(1);
        }

        semaphoreOperationStructure[0].sem_num = 0;
        semaphoreOperationStructure[0].sem_op = 1;
        semaphoreOperationStructure[0].sem_flg = 0;

        semaphoreOperationStructure[1].sem_num = 2;
        semaphoreOperationStructure[1].sem_op = 1;
        semaphoreOperationStructure[1].sem_flg = 0;

        semop(semaphoreSetId, semaphoreOperationStructure, 2);

        semaphoreOperationStructure[0].sem_num = 3;
        semaphoreOperationStructure[0].sem_op = 1;
        semop(semaphoreSetId, semaphoreOperationStructure, 1);


        printf("%s sent a message to queue: %s\nNumber of sent messages: %d\n", name, sendBuffer.data,
               semctl(semaphoreSetId, 3, GETVAL));

        sleep(5);
    }
}