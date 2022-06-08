#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cerrno>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace std;

pthread_mutex_t lock;

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

void *childThread(void* ptr);

void runChildThread(string childProcPath, string envFilePath, char** envp, int index) {

    string num = to_string(index);
    string procName = "child_" +  (index < 10 ? "0" + num : num);

    string* ptr = new string[2];

    ptr[0] = (string)procName;
    ptr[1] = (string)envFilePath;

    pthread_t threadId;

    if (pthread_create(&threadId, nullptr, childThread, ptr)!=0){
        printf("Error creating thread\n");
        exit(0);
    }
    pthread_join(threadId, nullptr);

}

int main(int argc, char** argv, char** envp) {
    printSortedEnvVars(envp);

    pthread_mutex_init(&lock, nullptr);

    string envFilePath = argv[1];

    int i = 0;

    while(true) {
        cout << "\nPress key...\t";

        rewind(stdin);

        char key;
        cin >> key;

        if (key == '+') {
            runChildThread(getenv("CHILD_PATH"), envFilePath, envp, i++);
        }

        if (key == '*') {
            runChildThread(getChildPath(environ), envFilePath, envp, i++);
        }

        if (key == '&') {
            runChildThread(getChildPath(envp), envFilePath, envp, i++);
            exit(0);
        }
    }
}

void readEnvVarsFromFile(string envFilePath) {
    ifstream file;

    file.open(envFilePath);

    if (!file.is_open()) {
        cout << "Unable to open file!" << endl;
        return;
    }

    string envVar;

    while (file >> envVar) {
        cout << envVar << '=';

        char* envValue = getenv(envVar.c_str());

        if (!envValue) {
            cout << '\n';
            continue;
        }

        cout << envValue << endl;
    }

    file.close();
}

void *childThread(void* ptr) {
    stringstream outputStream;
    outputStream << "\033[1;34m" <<
                 "\nProcess: " << ((string*)ptr)[0] << "\tppid = " << getppid()<<"\tpid = " <<getpid()<<"\ttid = "<<pthread_self() <<"\033[0m\n\n";
    string output = outputStream.str();
    pthread_mutex_lock(&lock);
    for (char i : output){
        cout<<i;
    }
    pthread_mutex_unlock(&lock);
    readEnvVarsFromFile(string(((string*)ptr)[1]));

    return nullptr;
}