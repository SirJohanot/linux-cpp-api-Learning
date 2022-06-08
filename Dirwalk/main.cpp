#include <iostream>
#include <cstring>
#include <vector>
#include <dirent.h>
#include <algorithm>

using namespace std;

struct Flags {
    bool l = false;
    bool d = false;
    bool f = false;
    bool s = false;
};

string getDirName(int argc, char **argv) {
    if (argc == 1) {
        return ".";
    }
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            return argv[i];
        }
    }
    return ".";
}

Flags getFlags(int argc, char *argv[]) {
    Flags flags;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            flags.l = true;
        }
        if (strcmp(argv[i], "-d") == 0) {
            flags.d = true;
        }
        if (strcmp(argv[i], "-f") == 0) {
            flags.f = true;
        }
        if (strcmp(argv[i], "-s") == 0) {
            flags.s = true;
        }
    }
    if (!(flags.f || flags.d || flags.l)) {
        flags.f = flags.d = flags.l = true;
    }
    return flags;
}

void dirwalk(const string &dirName, Flags flags, vector<string> &results) {
    DIR *dir = nullptr;
    struct dirent *entry = nullptr;
    dir = opendir(dirName.c_str());
    if (!dir) {
        cout << "Unable to open folder " + dirName + "!" << endl;
        exit(0);
    }
    while ((entry = readdir(dir))) {
        string entryName = entry->d_name;
        if (entryName == "." || entryName == "..") {
            continue;
        }
        string path = dirName + '/' + entryName;
        if (entry->d_type == DT_LNK && flags.l) {
            string pathToPrint = "LINK: " + path;
            results.push_back(pathToPrint);
        }
        if (entry->d_type == DT_DIR) {
            if (flags.d) {
                string pathToPrint = "DIRECTORY: " + path;
                results.push_back(pathToPrint);
            }
            dirwalk(path, flags, results);
        }
        if (entry->d_type == DT_REG && flags.f) {
            string pathToPrint = "FILE: " + path;
            results.push_back(pathToPrint);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    string dirName = getDirName(argc, argv);
    Flags flags = getFlags(argc, argv);
    vector<string> entries;
    dirwalk(dirName, flags, entries);
    if (flags.s) {
        sort(entries.begin(), entries.end());
    }
    for (const auto &entry: entries) {
        cout << entry << endl;
    }
    return 0;
}