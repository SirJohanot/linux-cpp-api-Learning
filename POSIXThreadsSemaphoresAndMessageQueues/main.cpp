#include<iostream>
#include <cstdlib>
#include<cstring>
#include<vector>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <cerrno>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <pthread.h>

using namespace std;

vector<pthread_t> consumersVector;
vector<pthread_t> producersVector;

pthread_mutex_t lock;

volatile unsigned int sentCounter;
volatile unsigned int receivedCounter;

#define MAX_SIZE 255

struct messageBuffer {
    long type;
    char data[MAX_SIZE];
};

union semun { // arg
    int val; //
    struct semid_ds *buf; //
    unsigned short *array; //
} arg;

string createChildProcessName(const string &str, int n) {
    stringstream ss;
    ss << str << n << '\0';
    return ss.str();
}

void *consumerMain(void* ptr);
void* producerMain(void* ptr);

int main(int argc, char *argv[]) {

    pthread_mutex_init(&lock, nullptr);

    int messageQueueId;
    int semaphoreSetId;

    key_t key = ftok(argv[0], 'a');
    printf(" key = %d\n", key);
    if ((messageQueueId = msgget(key, IPC_CREAT | 0666)) < 0) {
        perror("msgget");
        exit(1);
    }
    printf(" messageQueueId = %d\n", messageQueueId);

    if ((semaphoreSetId = semget(key, 2, IPC_CREAT | 0666)) < 0) {
        perror("semget");
        exit(1);
    }

    arg.val = MAX_SIZE;
    semctl(semaphoreSetId, 0, SETVAL, arg);
    arg.val = 0;
    semctl(semaphoreSetId, 1, SETVAL, arg);

    int consumerNumber = 0;
    int producerNumber = 0;
    char command;
    pthread_t newThreadId;

    char *args[] = {" ", argv[0], (char *) 0};

    while (true) {
        printf("1 - create new consumer\n2 - create new producer\ne - xit\n");

        cin >> command;
        switch (command) {
            case '1':
                args[0] = &createChildProcessName("consumer_", consumerNumber)[0];
                if (pthread_create(&newThreadId, nullptr, consumerMain, (void*)args)){
                    printf("Error creating consumer thread!\n");
                } else {
                    consumersVector.push_back(newThreadId);
                    consumerNumber++;
                }
                break;

            case '2':
                args[0] = &createChildProcessName("producer_", producerNumber)[0];
                if (pthread_create(&newThreadId, nullptr, producerMain, (void*)args)){
                    printf("Error creating producer thread!\n");
                } else {
                    producersVector.push_back(newThreadId);
                    producerNumber++;
                }
                break;


            case '-':
                if (producerNumber > 0) {
                    cout << "producer kill thread id = " << (int) producersVector.back() << endl;
                    pthread_cancel(producersVector.back());
                    producerNumber--;
                }
                if (consumerNumber > 0) {
                    cout << "consumer kill thread id = " << (int) consumersVector.back() << endl;
                    pthread_cancel(consumersVector.back());
                    consumerNumber--;
                }
                break;
            case 'e':
                for (unsigned int i: consumersVector) {
                    pthread_cancel(i);
                }
                for (unsigned int i: producersVector) {
                    pthread_cancel(i);
                }
                exit(0);
            default:
                break;
        }

    }
}

char *producerCreateMessage() {
//    printf("Producing message\n");
    int len = rand() % 50 + 1;
    char *s = (char *) calloc(len, 1);
    for (int i = 0; i < len; i++) {
        s[i] = (char) (rand() % 26 + 97);
        if (i + 1 == len) {
            s[i] = '\0';
        }
    }
//    printf("Produced message\n");
    return s;
}

void* producerMain(void* ptr){

    struct sembuf semaphoreOperationStructure{};

    char *name = ((char**)ptr)[0];
    key_t key = ftok(((char**)ptr)[1], 'a');
    int messageQueueId;
    int semaphoreSetId;
    messageBuffer sendBuffer{};

    if ((messageQueueId = msgget(key, IPC_EXCL | 0666)) < 0) {
        perror("msgget");
        exit(errno);
    }

    if ((semaphoreSetId = semget(key, 2, IPC_CREAT | 0666)) < 0) {
        perror("semget");
        exit(errno);
    }

    printf("\nChild name = %s, ", name);
    printf("child got messageQueueId = %d, ", messageQueueId);
    printf("got semaphoreSetId = %d\n", semaphoreSetId);

    while (true) {

        semaphoreOperationStructure.sem_num = 0;
        semaphoreOperationStructure.sem_op = -1;
        semaphoreOperationStructure.sem_flg = 0;

        semop(semaphoreSetId, &semaphoreOperationStructure, 1);
//        printf("Got semaphore: producer\n");
        pthread_mutex_lock(&lock);
//        printf("Locked mutex: producer\n");

        sendBuffer.type = 3;
        strcpy(sendBuffer.data, producerCreateMessage());
//        printf("Sending message\n");
        if (msgsnd(messageQueueId, &sendBuffer, strlen(sendBuffer.data) + 1, IPC_NOWAIT) < 0) {
            printf("%d, %ld, %s, %ld\n", messageQueueId, sendBuffer.type, sendBuffer.data, strlen(sendBuffer.data) + 1);
            perror("msgsnd");
            exit(1);
        }

//        printf("Incrementing counter\n");
        sentCounter++;
//        printf("Sending console message\n");
        printf("%s sent a message to queue: %s\nNumber of sent messages: %d\n", name, sendBuffer.data,
               sentCounter);

        pthread_mutex_unlock(&lock);

        semaphoreOperationStructure.sem_num = 1;
        semaphoreOperationStructure.sem_op = 1;
        semaphoreOperationStructure.sem_flg = 0;

        semop(semaphoreSetId, &semaphoreOperationStructure, 1);

        sleep(5);
    }
}

void* consumerMain(void* ptr){
    struct sembuf semaphoreOperationStructure{};
    
    char *name = ((char**)ptr)[0];
    key_t key = ftok(((char**)ptr)[1], 'a');

    int messageQueueId;
    if ((messageQueueId = msgget(key, IPC_EXCL | 0666)) < 0) {
        perror("msgget");
        exit(errno);
    }

    int semaphoreSetId;
    if ((semaphoreSetId = semget(key, 2, IPC_CREAT | 0666)) < 0) {
        perror("semget");
        exit(errno);
    }

    printf("\nChild name = %s, ", name);
    printf("child got messageQueueId = %d, ", messageQueueId);
    printf("got semaphoreSetId = %d\n", semaphoreSetId);

    messageBuffer readBuffer{};

    while (true) {

        semaphoreOperationStructure.sem_num = 1;
        semaphoreOperationStructure.sem_op = -1;
        semaphoreOperationStructure.sem_flg = 0;

        semop(semaphoreSetId, &semaphoreOperationStructure, 1);
        pthread_mutex_lock(&lock);

        if (msgrcv(messageQueueId, &readBuffer, MAX_SIZE + 1, 3, IPC_NOWAIT) < 0) {
            perror("msgrcv");
            exit(errno);
        }

        receivedCounter++;
        printf("%s got a message from queue: %s\nNumber of received messages: %u\n", name, readBuffer.data,
               receivedCounter);

        pthread_mutex_unlock(&lock);

        semaphoreOperationStructure.sem_num = 0;
        semaphoreOperationStructure.sem_op = 1;
        semaphoreOperationStructure.sem_flg = 0;

        semop(semaphoreSetId, &semaphoreOperationStructure, 1);

        sleep(5);
    }
}