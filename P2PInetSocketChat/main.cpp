#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

using namespace std;

void initParams(int argc, char *argv[], string &ip, int &port, string &userName, bool &server) {
    if (argc != 5) {
        cout << "The program should be run with the following parameters:" << argv[0]
             << " <your IP/target device IP> <your port/target device port> <username> <-s(server)/-j(join)>"
             << endl;
        exit(0);
    }
    ip = argv[1];
    cout << "IP extracted" << endl;
    port = stoi(argv[2]);
    cout << "port extracted" << endl;
    userName = argv[3];
    cout << "username extracted" << endl;
    string parameter = argv[4];
    if (parameter == "-s") {
        server = true;
        return;
    }
    if (parameter == "-j") {
        server = false;
        return;
    }
    cout << "The parameter has to be either -s to run as server or -j to join another user" << endl;
    exit(0);
}

void *displayUpdater(void *ptr) {
    int socketDescriptor = *(int *) ptr;
    off_t chatLength = lseek(socketDescriptor, 0, SEEK_END);
    char *chat;
    ssize_t bytesRead;

    system("clear");
    cout << "Chat start!" << endl;
    while (true) {
        chat = (char *) calloc(512, 1);
        bytesRead = read(socketDescriptor, chat, 512);
        cout << chat;
        if (bytesRead <= 0) {
            free(chat);
            break;
        }
        free(chat);
    }
    cout << endl << "New message [-1 to exit]: " << flush;
    lseek(socketDescriptor, 0, SEEK_SET);

    while (true) {
        if (lseek(socketDescriptor, 0, SEEK_END) != chatLength) {
            chatLength = lseek(socketDescriptor, 0, SEEK_END);
            lseek(socketDescriptor, 0, SEEK_SET);
            system("clear");
            cout << "Chat start!" << endl;
            while (true) {
                chat = (char *) calloc(512, 1);
                bytesRead = read(socketDescriptor, chat, 512);
                cout << chat;
                if (bytesRead < 512) {
                    free(chat);
                    break;
                }
                free(chat);
            }
            cout << endl << "New message [-1 to exit]: " << flush;
        }
        lseek(socketDescriptor, 0, SEEK_SET);
    }
}

int startupRoutine(string ip, int port, bool server) {
    int socketDescriptor;
    if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("could not create socket");
        exit(0);
    }
    cout << "socket created!" << endl;

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) < 0) {
        perror("binary IP conversion failed");
        exit(0);
    }

    if (server) {
        string socketName = "chatServer";
        if (bind(socketDescriptor, (sockaddr *) &address, sizeof(address)) < 0) {
            perror("socket bind failed");
            exit(0);
        }
        cout << "socket bound" << endl;
        if (listen(socketDescriptor, 1) < 0) {
            perror("socket listening failed");
            exit(0);
        }
        cout << "socket is listening..." << endl;
        int addrlen = sizeof(address);
        if ((socketDescriptor = accept(socketDescriptor, (sockaddr *) &address, (socklen_t *) &addrlen)) <
            0) {
            perror("server connection accepting failed");
            exit(0);
        }
        cout << "socket accepted a connection!" << endl;
    } else {
        if (connect(socketDescriptor, (sockaddr *) &address, sizeof(address)) < 0) {
            perror("peer connection Error");
            exit(0);
        }
        cout << "socket connected!" << endl;
    }

    pthread_t outThread;
    pthread_create(&outThread, nullptr, displayUpdater, (void *) &socketDescriptor);
    cout << "display thread created!" << endl;

    return socketDescriptor;
}

void exitRoutine(int socketDescriptor) {
    close(socketDescriptor);
}

int main(int argc, char *argv[]) {
    string ip;
    int port;
    string name;
    bool server;

    initParams(argc, argv, ip, port, name, server);

    int socketDescriptor = startupRoutine(ip, port, server);

    string message;
    while (true) {
        getline(cin, message);
        if (message == "-1") {
            break;
        }

        message = name + ": " + message + "\n";
        if (send(socketDescriptor, message.c_str(), strlen(message.c_str()), MSG_NOSIGNAL) == -1) {
            perror("could not send the message: ");
        }

    }
    exitRoutine(socketDescriptor);
    return 0;
}