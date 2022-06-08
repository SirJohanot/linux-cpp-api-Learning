#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <sstream>

using namespace std;

bool permission = true;
int childThreadsNumber = 0;
pthread_mutex_t lock;

struct CHILD {
    pthread_t threadId;
    bool permission;
} children[15] = {};

int getNum(const char *str) {
    int num = 0;
    for (int i = 1; str[i] && str[i] != '\n'; i++) {
        num = num * 10 + (str[i] - false);
    }
    return num;
}

void setPermissions(bool mod) {
    permission = mod;

    pthread_mutex_lock(&lock);
    for (int i = 0; i < childThreadsNumber; i++) {
        children[i].permission = mod;
    }
    pthread_mutex_unlock(&lock);

    printf("Now permission is set to %d\n", mod);
}

void killThreads() {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < childThreadsNumber; i++) {
        pthread_cancel(children[i].threadId);
    }
    childThreadsNumber = 0;
    printf("There are no more threads (%d)\n", childThreadsNumber);
    pthread_mutex_unlock(&lock);
}

void *childMain(void *ptr);

int main() {
    pthread_mutex_init(&lock, nullptr);

    pthread_t threadId;
    printf("Ready to work:\n");
    while (true) {
        char choice[5] = {};
        fgets(choice, 5, stdin);

        if (strlen(choice) - 1 == 1) {
            switch (choice[0]) {
                case '+': {

                    if (pthread_create(&threadId, nullptr, childMain, nullptr) != 0) {
                        printf("\nError");
                    } else {
//                        printf("Created %lu\n", threadId);
                        children[childThreadsNumber].threadId = threadId;
                        children[childThreadsNumber++].permission = permission;
                    }
                    break;
                }
                case '-': {
                    pthread_mutex_lock(&lock);
                    if (childThreadsNumber > 0) {
                        pthread_cancel(children[childThreadsNumber - 1].threadId);
                        childThreadsNumber--;
                        printf("Child process was closed, there are %d more process(es)\n", childThreadsNumber);
                    }
                    pthread_mutex_unlock(&lock);
                    break;
                }
                case 'k': {
                    killThreads();
                    break;
                }
                case 's': {
                    setPermissions(false);
                    break;
                }
                case 'g': {
                    setPermissions(true);
                    break;
                }
                case 'q': {
                    killThreads();
                    return 0;
                }
                default:
                    break;
            }
        } else {
            int num = getNum(choice);
            if (num > childThreadsNumber) {
                continue;
            }
            switch (choice[0]) {
                case 's': {
                    children[num - 1].permission = false; //disable child output
                    break;
                }
                case 'g': {
                    children[num - 1].permission = true; //enable child output
                    break;
                }
                case 'p': {
                    setPermissions(false);
                    sleep(5);
                    setPermissions(true);
                    printf("Threads enabled again\n");
                    break;
                }
                default:
                    break;
            }
        }
    }
}

volatile struct pair {
    int a;
    int b;
} memory;

void displayInfo() {
    stringstream ss;
    ss << "\nPPID: " << getppid() << " PID: " << getpid() << " TID: " << pthread_self() << "\n";
    string output = ss.str();
    for (char c: output) {
        cout << c;
    }
}

void printInfoIfAllowedTo() {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < childThreadsNumber; i++) {
        if (children[i].threadId == pthread_self() && children[i].permission) {
            displayInfo();
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

void *childMain(void *ptr) {
    printf("Child process was created (id %lu)\n", pthread_self());

    memory.a = 0;
    memory.b = 0;

    while (true) {
        sleep(3);
        printInfoIfAllowedTo();
        memory.a = 0;
        memory.b = 0;
        memory.a = 1;
        memory.b = 1;
    }
}