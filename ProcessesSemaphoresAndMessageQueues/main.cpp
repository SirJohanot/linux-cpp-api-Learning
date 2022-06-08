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

using namespace std;

vector<pid_t> consumersVector;
vector<pid_t> producersVector;

#define MAXSIZE 255
struct sembuf sembuf;

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

int main(int argc, char *argv[], char *envp[]) {

    int messageQueueId;
    int semaphoreSetId;

    key_t key = ftok(argv[0], 'a');
    printf(" key = %d\n", key);
    if ((messageQueueId = msgget(key, IPC_CREAT | 0666)) < 0) {
        perror("msgget");
        exit(1);
    }
    printf(" messageQueueId = %d\n", messageQueueId);

    if ((semaphoreSetId = semget(key, 5, IPC_CREAT | 0666)) < 0) {
        perror("semget");
        exit(1);
    }

    arg.val = 1;
    semctl(semaphoreSetId, 0, SETVAL, arg);
    arg.val = 255;
    semctl(semaphoreSetId, 1, SETVAL, arg);
    arg.val = 0;
    semctl(semaphoreSetId, 2, SETVAL, arg);
    arg.val = 0;
    semctl(semaphoreSetId, 3, SETVAL, arg); //send count
    arg.val = 0;
    semctl(semaphoreSetId, 4, SETVAL, arg); //get count

    int consumerNumber = 0;
    int producerNumber = 0;
    char command;
    pid_t newProcessId;

    char *args[] = {" ", argv[0], (char *) 0};

    while (true) {
        printf("1 - create new consumer\n2 - create new producer\ne - xit\n");

        cin >> command;
        switch (command) {
            case '1':
                newProcessId = fork();
                if (newProcessId == -1) {
                    fprintf(stdout, "Error occured, error code - %d\n", errno);
                    exit(errno);
                }
                if (newProcessId == 0) {
                    args[0] = &createChildProcessName("consumer_", consumerNumber)[0];
                    execve("./consumer", args, envp);
                }
                consumersVector.push_back(newProcessId);
                consumerNumber++;
                break;

            case '2':
                newProcessId = fork();
                if (newProcessId == -1) {
                    fprintf(stdout, "Error occured, error code - %d\n", errno);
                    exit(errno);
                }
                if (newProcessId == 0) {
                    args[0] = &createChildProcessName("producer_", producerNumber)[0];
                    execve("./producer", args, envp);
                }
                producersVector.push_back(newProcessId);
                producerNumber++;
                break;


            case '-':
                if (producerNumber > 0) {
                    cout << "producer kill pid = " << (int) consumersVector.back() << endl;
                    kill(consumersVector.back(), SIGKILL);
                    producerNumber--;
                }
                if (consumerNumber > 0) {
                    cout << "consumer kill pid = " << (int) consumersVector.back() << endl;
                    kill(consumersVector.back(), SIGKILL);
                    consumerNumber--;
                }
                break;
            case 'e':
                for (int i: consumersVector) {
                    kill(i, SIGKILL);
                }
                for (int i: producersVector) {
                    kill(i, SIGKILL);
                }
                exit(0);
            default:
                break;
        }

    }
}