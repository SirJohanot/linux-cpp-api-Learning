#include <iostream>
#include <sys/shm.h>
#include <cstring>
#include <pthread.h>

using namespace std;

void initParams(int argc, char *argv[], int &shmKey, string &userName) {
    if (argc != 3) {
        cout << "The program should be run with the following parameters:" << argv[0] << " <shmKey> <userName>"
             << endl;
        exit(0);
    }
    shmKey = stoi(argv[1]);
    cout << "shmKey extracted" << endl;
    userName = argv[2];
    cout << "userName extracted" << endl;
}

char *chatHistory;
pthread_mutex_t lock;

void *displayUpdater(void *ptr) {
    pthread_mutex_lock(&lock);
    long chatLength = strlen(chatHistory);
    system("clear");
    cout << "Chat start!" << endl << chatHistory << "\nNew message [-1 to exit]: " << flush;
    pthread_mutex_unlock(&lock);
    while (true) {
        if (strlen(chatHistory) != chatLength) {
            pthread_mutex_lock(&lock);
            chatLength = strlen(chatHistory);
            system("clear");
            cout << "Chat start!" << endl << chatHistory << "\nNew message [-1 to exit]: " << flush;
            pthread_mutex_unlock(&lock);
        }
    }
}

void writeMessage(string message) {
    cout << "writing message to memory..." << endl;
    strcat(chatHistory, message.c_str());
}

int startupRoutine(int shmKey) {
    int shmId = shmget(shmKey, 27, IPC_CREAT | 0666);
    if (shmId < 0) {
        cout << "error getting/creating shared memory" << endl;
        exit(0);
    }
    chatHistory = (char *) shmat(shmId, nullptr, 0);
    if (chatHistory == (char *) -1) {
        cout << "error attaching memory" << endl;
        exit(0);
    }
    cout << "memory attached" << endl;
    pthread_t updaterThread;
    pthread_mutex_init(&lock, nullptr);
    pthread_create(&updaterThread, nullptr, displayUpdater, nullptr);
    return shmId;
}

void exitRoutine(int shmId) {
    pthread_mutex_lock(&lock);
    shmctl(shmId, IPC_RMID, nullptr);
    shmdt(chatHistory);
    cout << "memory detached successfully" << endl;
    pthread_mutex_unlock(&lock);
    exit(0);
}

int main(int argc, char *argv[]) {
    int shmKey;
    string userName;
    initParams(argc, argv, shmKey, userName);
    int shmId = startupRoutine(shmKey);
    string userMessage;
    while (true) {
        getline(cin, userMessage);
        if (userMessage == "-1") {
            exitRoutine(shmId);
        }
        if (userMessage.length() > 0) {
            userMessage = userName + ": " + userMessage + "\n";
            pthread_mutex_lock(&lock);
            writeMessage(userMessage);
            pthread_mutex_unlock(&lock);
        }
    }
    return 0;
}
