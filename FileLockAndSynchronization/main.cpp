#include <cstdio>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

const string storageFilePath = "storage.txt";

struct product {
    int id;
    char name[255];
    int code;
    unsigned int amount;
    bool reserved;
};

void productQuery(product &buffer) {
    cout << endl << "name: " << flush;
    cin >> buffer.name;
    cout << endl << "code: " << flush;
    cin >> buffer.code;
    cout << endl << "amount: " << flush;
    cin >> buffer.amount;
    cout << endl << "reserved (0-false, 1-true): " << flush;
    cin >> buffer.reserved;
    cout << endl;
}

void displayProduct(product &prod) {
    cout << endl << "id: " << prod.id << endl;
    cout << "name: " << prod.name << endl;
    cout << "code: " << prod.code << endl;
    cout << "amount: " << prod.amount << endl;
    cout << "reserved: " << prod.reserved << endl;
}

int getFileDescriptor(const char *filePath, int flags) {
    int descriptor = open(filePath, flags);
    if (descriptor == -1) {
        perror("could not open file!");
        exit(0);
    }
    return descriptor;
}

void applyFlockToFile(int descriptor, short int flockType, short int whence, int length) {
    flock fileLock = {flockType, whence, 0, length};
    if (fcntl(descriptor, F_SETLKW, &fileLock) == -1) {
        perror("could not lock file");
        exit(0);
    }
}

void add() {
    int storageDescriptor = getFileDescriptor(storageFilePath.c_str(), O_RDWR);
    product buffer{};
    for (int i = 0; true; i++) {

        applyFlockToFile(storageDescriptor, F_WRLCK, SEEK_CUR, sizeof(product));

        if (read(storageDescriptor, &buffer, sizeof(buffer)) == 0) {
            buffer.id = i + 1;
            productQuery(buffer);
            write(storageDescriptor, &buffer, sizeof(buffer));
            lseek(storageDescriptor, -sizeof(product), SEEK_CUR);
            applyFlockToFile(storageDescriptor, F_UNLCK, SEEK_CUR, sizeof(product));
            close(storageDescriptor);
            return;
        } else {
            if (buffer.id == -1) {
                lseek(storageDescriptor, -sizeof(buffer), SEEK_CUR);
                buffer.id = i + 1;
                productQuery(buffer);
                write(storageDescriptor, &buffer, sizeof(buffer));

                applyFlockToFile(storageDescriptor, F_UNLCK, SEEK_CUR, -sizeof(product));
                close(storageDescriptor);
                return;
            }
        }

        applyFlockToFile(storageDescriptor, F_UNLCK, SEEK_CUR, -sizeof(product));
    }
}

void remove() {
    int index;
    product buffer{};
    cout << "id of the product to delete: " << flush;
    cin >> index;
    cout << endl;
    if (index < 1) {
        cout << "id cannot be less than 1" << endl;
        return;
    }
    int storageDescriptor = getFileDescriptor(storageFilePath.c_str(), O_RDWR);
    lseek(storageDescriptor, (index - 1) * sizeof(product), SEEK_SET);
    applyFlockToFile(storageDescriptor, F_WRLCK, SEEK_CUR, sizeof(product));
    if (read(storageDescriptor, &buffer, sizeof(buffer)) == 0) {
        cout << "eof reached" << endl;
        close(storageDescriptor);
        return;
    }
    lseek(storageDescriptor, -sizeof(buffer), SEEK_CUR);
    buffer.id = -1;
    write(storageDescriptor, &buffer, sizeof(buffer));

    applyFlockToFile(storageDescriptor, F_UNLCK, SEEK_CUR, -sizeof(product));

    close(storageDescriptor);
}

product getById() {
    int index;
    product buffer{};
    buffer.id = -1;
    cout << "id of target product: ";
    cin >> index;
    if (index < 1) {
        cout << "id must not be less than 1" << endl;
        return buffer;
    }

    int storageDescriptor = getFileDescriptor(storageFilePath.c_str(), O_RDWR);
    lseek(storageDescriptor, (index - 1) * sizeof(product), SEEK_SET);

    applyFlockToFile(storageDescriptor, F_WRLCK, SEEK_CUR, sizeof(product));

    if (read(storageDescriptor, &buffer, sizeof(buffer)) == 0 || buffer.id < 1) {
        cout << "a product by that id does not yet exist" << endl;
    }

    applyFlockToFile(storageDescriptor, F_UNLCK, SEEK_CUR, -sizeof(product));

    close(storageDescriptor);
    return buffer;
}

void put() {
    int targetId;
    product buffer{};
    cout << "input id to edit:";
    cin >> targetId;
    if (targetId < 1) {
        cout << "target id must not be less than 1" << endl;
        return;
    }
    buffer.id = targetId;
    int storageDescriptor = getFileDescriptor(storageFilePath.c_str(), O_RDWR);

    flock fl = {F_WRLCK, 0, 0, 0};
    fcntl(storageDescriptor, F_SETLKW, &fl);

    lseek(storageDescriptor, (targetId - 1) * sizeof(product), SEEK_SET);
    if (read(storageDescriptor, &buffer, sizeof(buffer)) == 0) {
        cout << "a product by that id does not yet exist" << endl;
        close(storageDescriptor);
        return;
    }

    displayProduct(buffer);
    productQuery(buffer);

    lseek(storageDescriptor, -sizeof(product), SEEK_CUR);
    write(storageDescriptor, &buffer, sizeof(buffer));

    fl.l_type = F_UNLCK;
    fcntl(storageDescriptor, F_SETLKW, &fl);

    close(storageDescriptor);
}

int search() {
    int action;
    int code;
    string name;

    cout << endl << " 1 - search by code" << endl;
    cout << "2 - search by name" << endl;
    cin >> action;
    switch (action) {
        case 1:
            cout << "code: ";
            cin >> code;
            break;
        case 2:
            cout << "name: ";
            cin >> name;
            break;
        default:
            return -1;
    }

    int storageDescriptor = getFileDescriptor(storageFilePath.c_str(), O_RDWR);
    product buffer{};
    for (int i = 0; true; i++) {
        applyFlockToFile(storageDescriptor, F_WRLCK, SEEK_CUR, sizeof(product));

        if (read(storageDescriptor, &buffer, sizeof(buffer)) == 0) {
            close(storageDescriptor);
            return -1;
        } else {
            if (buffer.id != -1 && ((action == 1 && buffer.code == code) || (action == 2 && buffer.name == name))) {
                close(storageDescriptor);
                return i + 1;
            }
        }

        applyFlockToFile(storageDescriptor, F_UNLCK, SEEK_CUR, -sizeof(product));
    }

    close(storageDescriptor);
    return -1;
}

int main() {
    int storageDescriptor = getFileDescriptor(storageFilePath.c_str(), O_CREAT);
    close(storageDescriptor);

    product productInstance{};
    int action;
    while (true) {
        fflush(stdin);
        rewind(stdin);
        cout << "0 - quit" << endl;
        cout << "1 - add" << endl;
        cout << "2 - get by id" << endl;
        cout << "3 - remove" << endl;
        cout << "4 - put" << endl;
        cout << "5 - search by name/code" << endl;
        cin >> action;

        switch (action) {
            case 0:
                return 0;
            case 1:
                add();
                break;
            case 2:
                productInstance = getById();
                if (productInstance.id != -1) {
                    displayProduct(productInstance);
                }
                break;
            case 3:
                remove();
                break;
            case 4:
                put();
                break;
            case 5: {
                int b = search();
                if (b != -1) {
                    cout << "id of queried product: " << b << endl;
                } else {
                    cout << "no such product found!" << endl;
                }
                break;
            }
        }
    }
}