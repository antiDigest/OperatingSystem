/*
    @author: antriksh
    Version 0: 2/5/2018
*/

#include "header/Socket.hpp"

class Server: protected Socket {
    /*
        Server:
        * Manages files and file paths
        * Takes client requests to read and write
        * Does not do anything for maintaining mutual exclusion
    */

private:
     string directory;   

public:
    Server(char* argv[], vector<ProcessInfo> clients, vector<ProcessInfo> servers)
    : Socket(argv, clients, servers) {

        id = argv[1];
        logger.open("logs/" + this->id + ".txt", ios::app | ios::out);

        directory = argv[3];

        this->sayHello();
    }

    void checkMessage(Message *message) {
        if (message->message == "hi") {

            ProcessInfo client = getFd(message);
            try {
                int fd = connectTo(client.hostname, client.port);
                this->send(this->personalfd, fd, "hello", client.processID);
                close(fd);
            } catch (const char* e) {
                Logger(e, this->clock);
                return;
            }
        } else {
            this->checkReadWrite(message);
        }
    }

    void checkReadWrite(Message *message) {
        switch (message->readWrite) {
        case 0: {
            this->send(message->destination, message->source, "received", message->sourceID);
            return;
        }
        case 1: {
            string line = readFile(message->fileName);
            writeMessage(message, line, 1, message->fileName);
            break;
        }
        case 2: {
            writeToFile(message->fileName, message->message);
            writeMessage(message, message->message, 2, message->fileName);
            break;
        }
        case 3: {
            vector<string> files = this->readDirectory(directory);
            writeMessage(message, makeFileTuple(files), 3);
            break;
        }
        default: {
            this->send(message->destination, message->source, "Not making sense !", message->sourceID);
            return;
        }
        }
    }

    string readFile(string INPUT_FILE) {
        ifstream file(INPUT_FILE);

        string line;
        while (getline(file, line)) {}

        Logger("[RETURN READ FROM FILE]:" + line, this->clock);

        return line;
    }

    void writeToFile(string INPUT_FILE, string message) {
        ofstream file;
        file.open(INPUT_FILE, ios::app | ios::out);
        file << message;
        file.close();
    }

    vector<string> readDirectory(string name) {
        vector<string> v;
        DIR* dirp = opendir(name.c_str());
        struct dirent *dp;
        while ((dp = readdir(dirp)) != NULL) {
            v.push_back(dp->d_name);
            // cout << dp->d_name << endl;
        }
        closedir(dirp);

        return v;
    }

};

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "usage %s ID port directory_path\n", argv[0]);
        exit(1);
    }

    vector<ProcessInfo> allClients, allServers;
    allClients = readClients(allClients, "clients.csv");
    allServers = readClients(allServers, "servers.csv");
    Server *server = new Server(argv, allClients, allServers);

    logger.close();
    return 0;
}