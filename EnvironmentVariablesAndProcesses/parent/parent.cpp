#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <errno.h>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

void printSortedEnvVars(char** envp) {
    vector<char*> envVars;

    for (char **env = &*envp; *env; env++) {
        envVars.push_back(*env);
    }

    sort(envVars.begin(), envVars.end(), [](const char* s1, const char* s2) {
        return strcmp(s1, s2) < 0;
    });

    for (char* var : envVars) {
        cout << var <<endl;
    }
}

string getChildPath(char** envp) {
    string childPathEnv;

    for (char **env = &*envp; *env; env++) {
        char* withChildPath = strstr(*env, "CHILD_PATH");

        if (withChildPath) {
            childPathEnv = *env;
            break;
        }
    }

    return childPathEnv.substr(childPathEnv.find("=") + 1);
}


void runChildProcess(string childProcPath, string envFilePath, char** envp, int index) {
    int procStatus;

    pid_t pid = fork();

    if (pid == -1) {
        cout << "Error occured!" << strerror(errno);
    }

    if (pid == 0) {
        string num = to_string(index);
        string procName = "child_" +  (index < 10 ? "0" + num : num);

        char** argv = new char*[2];

        argv[0] = (char*)procName.c_str();
        argv[1] = (char*)envFilePath.c_str();

        execve(childProcPath.c_str(), argv, envp);

        delete[] argv;
    }

    wait(&procStatus);
}

int main(int argc, char** argv, char** envp) {
    printSortedEnvVars(envp);

    string envFilePath = argv[1];

    int i = 0;

    while(true) {
        cout << "\nPress key...\t";

        rewind(stdin);

        char key;
        cin >> key;

        if (key == '+') {
            runChildProcess(getenv("CHILD_PATH"), envFilePath, envp, i++);
        }

        if (key == '*') {
            runChildProcess(getChildPath(environ), envFilePath, envp, i++);
        }

        if (key == '&') {
            runChildProcess(getChildPath(envp), envFilePath, envp, i++);
            exit(0);
        }
    }

    return 0;
}