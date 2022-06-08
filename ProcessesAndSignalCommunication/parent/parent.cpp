#include<cstdio>
#include<string>
#include<cstring>
#include<cstdlib>
#include<unistd.h>
#include<csignal>
#include<cerrno>

using namespace std;

bool permission = true;
int childProcessesNumber = 0;

struct CHILD {
    pid_t pid;
    bool permission;
} children[15] = {};

int getNum(const char *str) {
    int num = 0;
    for (int i = 1; str[i] && str[i] != '\n'; i++) {
        num = num * 10 + (str[i] - false);
    }
    return num;
}

void setPermissions(char mod) {
    permission = mod;
    printf("Now permission is set to %c\n", mod);
    for (int i = 0; i < childProcessesNumber; i++) {
        children[i].permission = mod;
    }
}

void killProcesses() {
    for (int i = 0; i < childProcessesNumber; i++) {
        kill(children[i].pid, SIGTERM);
    }
    childProcessesNumber = 0;
    printf("There are no more processes (%d)\n", childProcessesNumber);
}

void handler(int sig, siginfo_t *si, void *context) {
    switch (sig) {
        case SIGALRM:
            setPermissions(true);
            printf("Processes enabled again\n");
            break;
        case SIGUSR1:
            for (int i = 0; i < childProcessesNumber; i++) {
                if (children[i].pid == si->si_pid) {
                    if (children[i].permission) {
                        kill(si->si_pid, SIGUSR2);
                    }
                    break;
                }
            }
    }
}

struct sigaction buildSigAction() {
    struct sigaction signalStruct{};
    signalStruct.sa_sigaction = handler;
    sigemptyset(&signalStruct.sa_mask);
    signalStruct.sa_flags = SA_SIGINFO;
    return signalStruct;
}

void setSigActionOrExit(int sig, struct sigaction structSigAction, const string &signalName) {
    if (sigaction(sig, &structSigAction, nullptr) == -1) {
        string errorMessage = "Parent: Error in sigaction (";
        errorMessage.append(signalName);
        errorMessage.append(")!\n");
        perror(errorMessage.c_str());
        exit(errno);
    }
}

int main() {
    struct sigaction signalStruct = buildSigAction();

    setSigActionOrExit(SIGALRM, signalStruct, "SIGALRM");
    setSigActionOrExit(SIGUSR1, signalStruct, "SIGUSR1");

    string program = "/home/johanot/CLionProjects/laba4/child/child";
    pid_t pid;
    printf("Ready to work:\n");
    while (true) {
        char choice[5] = {};
        fgets(choice, 5, stdin);

        if (strlen(choice) - 1 == 1) {
            switch (choice[0]) {
                case '+': {
                    pid = fork();
                    if (pid == -1) {
                        printf("\nError");
                    }
                    if (pid == 0) {
                        string permissionLine = permission ? "true" : "false";
                        if (execl(program.c_str(), permissionLine.c_str(), NULL) == -1) {
                            perror("PARENT: Error in execl\n");
                            exit(errno);
                        }
                    } else {
                        children[childProcessesNumber].pid = pid;
                        children[childProcessesNumber++].permission = permission;
                    }
                    break;
                }
                case '-': {
                    if (childProcessesNumber > 0) {
                        kill(children[childProcessesNumber - 1].pid, SIGTERM);
                        childProcessesNumber--;
                        printf("Child process was closed, there are %d more process(es)\n", childProcessesNumber);
                    }
                    break;
                }
                case 'k': {
                    killProcesses();
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
                    killProcesses();
                    return 0;
                }
                default:
                    break;
            }
        } else {
            int num = getNum(choice);
            if (num > childProcessesNumber) {
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
                    kill(children[num - 1].pid, SIGUSR2);
                    alarm(5);
                    break;
                }
                default:
                    break;
            }
        }
    }
}