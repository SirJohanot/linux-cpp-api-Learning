#include<cstdio>
#include<cstdlib>
#include<csignal>
#include<unistd.h>
#include<cerrno>
#include <string>

using namespace std;

volatile struct pair {
    int a;
    int b;
} memory;

void handler(int sig) {
    switch (sig) {
        case SIGALRM:
            kill(getppid(), SIGUSR1);
            alarm(3);
            break;
        case SIGUSR1:
        case SIGUSR2:
            printf("\nPPID: %d PID: %d\n", getppid(), getpid());
            break;
        case SIGTERM:
            exit(0);
    }
}

struct sigaction buildSigaction() {
    struct sigaction signal_struct{};

    signal_struct.sa_handler = handler;
    sigemptyset(&signal_struct.sa_mask);
    signal_struct.sa_flags = 0;

    return signal_struct;
}

void setSigActionOrExit(int sig, struct sigaction structSigAction, const string &signalName) {
    if (sigaction(sig, &structSigAction, nullptr) == -1) {
        string errorMessage = "CHILD: Error in sigaction (";
        errorMessage.append(signalName);
        errorMessage.append(")!\n");
        perror(errorMessage.c_str());
        exit(errno);
    }
}


int main(int argc, char *argv[]) {
    printf("Child process was created (mod %s)\n", argv[0]);

    struct sigaction structSigAction = buildSigaction();

    setSigActionOrExit(SIGALRM, structSigAction, "SIGALRM");
    setSigActionOrExit(SIGUSR1, structSigAction, "SIGUSR1");
    setSigActionOrExit(SIGUSR2, structSigAction, "SIGUSR2");
    setSigActionOrExit(SIGTERM, structSigAction, "SIGTERM");

    memory.a = 0;
    memory.b = 0;
    alarm(3);

    while (true) {
        memory.a = 0;
        memory.b = 0;
        memory.a = 1;
        memory.b = 1;
    }
}