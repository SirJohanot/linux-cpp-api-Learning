#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>

using namespace std;

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

int main(int argc, char** argv) {
    cout << "\033[1;34m"
         << "\nProcess: " + string(argv[0]) << "\t"
         << "ppid = " << getppid()
         << "\tpid = " << getpid() << "\033[0m" << endl << endl;

    readEnvVarsFromFile(string(argv[1]));

    return 0;
}