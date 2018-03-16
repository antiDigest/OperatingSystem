/*
    @author: antriksh
    Version 0: 3/14/2018
*/

#include "header/MetaInfo.hpp"
#include "header/Socket.hpp"

// M-Server:
//         * Manages files and file paths
//         * Takes client requests to read and write
//         * Does not do anything for maintaining mutual exclusion
class Mserver : protected Socket {
   public:
    Mserver(char* argv[]) : Socket(argv) {
        directory = ".meta";
        const int dir_err =
            mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    // Infinite thread to accept connection and detach a thread as
    // a receiver and checker of messages
    void listener() {
        while (1) {
            // Accept a connection with the accept() system call
            int newsockfd =
                accept(personalfd, (struct sockaddr*)&cli_addr, &clilen);
            if (newsockfd < 0) {
                error("ERROR on accept");
            }
            Logger("New connection " + to_string(newsockfd));

            std::thread connectedThread(&Mserver::processMessages, this,
                                        newsockfd);
            connectedThread.detach();
        }
    }

    // Starts as a thread which receives a message and checks the message
    // @newsockfd - fd socket stream from which message would be received
    void processMessages(int newsockfd) {
        try {
            Message* message = this->receive(newsockfd);
            close(newsockfd);
            this->checkMessage(message);
        } catch (const char* e) {
            Logger(e);
        }
    }

    // Checks the message for different types of incoming messages
    // 1. hi - just a hello from some client/server (not in use)
    // 2. something else (means a request to be granted)
    // @m - Message just received
    // @newsockfd - socket stream it was received from
    void checkMessage(Message* m) {
        if (m->type == "heartbeat") {
            registerHeartBeat(m);
        } else {
            this->checkReadWrite(m);
        }
        throw "BREAKING CONNECTION";
    }

    // Checks the request type
    // 1 - Read: replies with meta-data for the file
    // 2 - Write: replies with meta-data for the file
    // 3 - Enquiry: replies with a list of files
    // @m - Message just received
    // @newsockfd - socket stream it was received from
    void checkReadWrite(Message* m) {
        switch (m->readWrite) {
            case 1: {
                checkRead(m);
                break;
            }
            case 2: {
                checkWrite(m);
                break;
            }
            case 3: {
                vector<string> files = readDirectory(directory);
                connectAndReply(m, "enquiry", makeFileTuple(files));
                break;
            }
            default: {
                string line = "UNRECOGNIZED MESSAGE !";
                connectAndReply(m, "", line);
                break;
            }
        }
    }

    // Checks what response should be given if the client wants to read a file
    void checkRead(Message* m) {
        try {
            if (!exists(m->fileName)) {
                connectAndReply(m, "FAILED", "FAILED");
            } else {
                string line = this->readFile(m->fileName);
                cout << line << endl;
                MetaInfo* meta = stringToInfo(line);
                ProcessInfo server = findInVector(allServers, meta->server);
                if (getAlive(server)) {
                    connectAndReply(m, "meta", line);
                } else {
                    connectAndReply(m, "FAILED", "FAILED");
                }
            }
        } catch (char* e) {
            Logger(e);
            connectAndReply(m, "FAILED", "FAILED");
        }
    }

    // Checks what response should be given if the client wants to write to a
    // file
    void checkWrite(Message* m) {
        try {
            if (!exists(m->fileName)) {
                createNewChunk(m, "1");
            } else {
                string line = this->readFile(m->fileName);
                cout << line << endl;
                MetaInfo* meta = stringToInfo(line);
                ProcessInfo server = findInVector(allServers, meta->server);
                int messageSize = m->message.length();
                if (getAlive(server)) {
                    if (messageSize > (8192 - meta->size)) {
                        string chunkName = to_string(stoi(meta->chunkName) + 1);
                        createNewChunk(m, chunkName);
                    } else {
                        connectAndReply(m, "meta", line);
                    }
                } else {
                    connectAndReply(m, "FAILED", "FAILED");
                }
            }
        } catch (char* e) {
            Logger(e);
            connectAndReply(m, "FAILED", "FAILED");
        }
    }

    // Updating meta-data in the meta Directory
    string updateMetaData(ProcessInfo server, string name, string chunkName,
                          int messageSize) {
        MetaInfo* newMeta =
            new MetaInfo(name, chunkName, server.processID, messageSize, 1);
        string line = infoToString(newMeta);
        writeToFile(name, line);
        return line;
    }

    // Creating new chunk
    void createNewChunk(Message* m, string chunkName) {
        int messageSize = m->message.length();
        ProcessInfo server = randomSelect(allServers);
        string line =
            updateMetaData(server, m->fileName, chunkName, messageSize);
        connectAndSend(server.processID, "create", line, 2, m->fileName);
        connectAndReply(m, "meta", line);
    }

    // Enquiry: replies with a list of files
    vector<string> readDirectory(string name) {
        vector<string> v;
        DIR* dirp = opendir(name.c_str());
        struct dirent* dp;
        while ((dp = readdir(dirp)) != NULL) {
            v.push_back(dp->d_name);
        }
        closedir(dirp);
        return v;
    }

    // TODO: Need atomic variables, @Shriroop help
    void registerHeartBeat(Message* m) {
        ProcessInfo p = findInVector(allServers, m->sourceID);
        Logger("[HEARTBEAT] " + m->sourceID);
        setAlive(p);
    }

    // TODO: Need atomic variables, @Shriroop help
    void checkAlive() {
        while (1) {
            sleep(3);
            for (ProcessInfo server : this->allServers) {
                if (getAlive(server)) {
                    float alive = (time(NULL) - getAliveTime(server));
                    Logger("[" + server.processID + "]: " + to_string(alive) +
                           "::" + to_string(getAlive(server)));
                    if (alive >= 15.0) {
                        Logger("[" + server.processID +
                               "]: " + to_string(getAlive(server)));
                        resetAlive(server);
                    }
                }
            }
        }
    }

    void setAlive(ProcessInfo p) {
        // ProcessInfo p = process.load();
        p.alive = true;
        p.aliveTime = time(NULL);
        // process.store(p);
    }

    void resetAlive(ProcessInfo p) {
        // ProcessInfo p = process.load();
        p.alive = false;
        // process.store(p);
    }

    bool getAlive(ProcessInfo p) {
        // ProcessInfo p = process.load();
        return p.alive;
    }

    time_t getAliveTime(ProcessInfo p) {
        // ProcessInfo p = process.load();
        return p.aliveTime;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "usage %s ID port directory_path\n", argv[0]);
        exit(1);
    }

    Mserver* server = new Mserver(argv);

    std::thread listenerThread(&Mserver::listener, server);
    // std::thread aliveThread(&Mserver::checkAlive, server);

    listenerThread.join();
    // aliveThread.join();

    logger.close();
    return 0;
}