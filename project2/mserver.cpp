/*
    @author: antriksh
    Version 0: 3/14/2018
*/

#include "header/MetaInfo.hpp"
#include "header/Socket.h"

// M-Server:
//         * Manages files and file paths
//         * Takes client requests to read and write
//         * Does not do anything for maintaining mutual exclusion
class Mserver : public Socket {
   private:
    vector<File*> files;

   public:
    Mserver(char* argv[]) : Socket(argv) {
        files = readFileInfo(files, "csvs/files.csv");
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

            std::thread connectedThread(&Mserver::processMessages, this,
                                        newsockfd);
            connectedThread.detach();
        }
    }

    // Starts as a thread which receives a message and checks the message
    // @newsockfd - fd socket stream from which message would be received
    void processMessages(int newsockfd) {
        // while (1) {
        try {
            Message* message = receive(newsockfd);
            close(newsockfd);
            checkMessage(message);
        } catch (const char* e) {
            Logger(e);
            close(newsockfd);
            // break;
        }
        // }
    }

    // Checks the message for different types of incoming messages
    // 1. heartbeat - just a hello from some server
    // 2. something else (means a request to be granted)
    // @m - Message just received
    // @newsockfd - socket stream it was received from
    void checkMessage(Message* m) {
        if (m->type == "heartbeat") {
            int index = findServerIndex(allServers, m->sourceID);
            registerHeartBeat(m, index);
        } else {
            checkReadWrite(m);
            throw "BREAKING CONNECTION";
        }
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
            default: {
                string line = "UNRECOGNIZED MESSAGE !";
                connectAndReply(m, "", line);
                break;
            }
        }

        updateCsv("csvs/files.csv", files);
    }

    // Checks what response should be given if the client wants to read a file
    void checkRead(Message* m) {
        try {
            string chunkName = to_string(getChunkNum(m->offset));
            string name = getChunkFile(m->fileName, chunkName);
            ProcessInfo server = findFileServer(allServers, name);
            replyMeta(m, m->fileName, chunkName, server);
        } catch (char* e) {
            Logger(e);
            Logger("[FAILED]");
            connectAndReply(m, "FAILED", "FAILED");
        }
    }

    // Checks what response should be given if the client wants to write to a
    // file and then read from it
    void checkWrite(Message* m) {
        try {
            File* file;
            file = findInVector(files, m->fileName);
            if (file == NULL) {
                file = createNewFile(m, new File);
            }

            string chunkName = to_string(file->chunks - 1);
            string name = getChunkFile(file->name, chunkName);

            ProcessInfo server = findFileServer(allServers, name);

            int messageSize = m->message.length();
            int chunkSize = getOffset(file->size);

            if (messageSize > (CHUNKSIZE - chunkSize)) {
                sizeGreater(m, file, chunkName, server, chunkSize, messageSize);
            } else {
                sizeSmaller(m, file, chunkName, server, 0, messageSize);
            }
        } catch (char* e) {
            Logger(e);
            Logger("[FAILED]");
            connectAndReply(m, "FAILED", "FAILED");
        }
    }

    File* createNewFile(Message* m, File* file) {
        Logger("[Creating new file]: " + m->fileName);
        file->name = m->fileName;
        createNewChunk(file, to_string(file->chunks));
        files.push_back(file);

        return file;
    }

    void sizeGreater(Message* m, File* file, string chunkName,
                     ProcessInfo server, int chunkSize, int messageSize) {
        int sizeThisChunk = messageSize - (CHUNKSIZE - chunkSize);
        int sizeNewChunk = messageSize - sizeThisChunk;

        sizeSmaller(m, file, chunkName, server, 0, sizeThisChunk);

        chunkName = to_string(stoi(chunkName) + 1);
        ProcessInfo newServer = createNewChunk(file, chunkName, sizeNewChunk);
        sizeSmaller(m, file, chunkName, newServer, sizeThisChunk, sizeNewChunk);
    }

    void sizeSmaller(Message* m, File* file, string chunkName,
                     ProcessInfo server, int offset, int byteCount) {
        m->offset = offset;
        m->byteCount = byteCount;
        replyMeta(m, file->name, chunkName, server);
    }

    void replyMeta(Message* m, string fileName, string chunkName,
                   ProcessInfo server) {
        MetaInfo* meta = new MetaInfo(fileName, chunkName, server.processID);
        string line = infoToString(meta);

        if (server.getAlive()) {
            connectAndReply(m, "meta", line);
        } else {
            connectAndReply(m, "FAILED", "FAILED");
        }
    }

    // Creating new chunk
    void createNewChunk(File* file, string chunkName) {
        ProcessInfo server = randomSelect(allServers);
        Logger("[Creating new file chunk at]: " + server.processID);
        string fileName = getChunkFile(file->name, chunkName);
        connectAndSend(server.processID, "create", "", 2, fileName);
        updateMetaData(file, server);
    }

    // Creating new chunk
    ProcessInfo createNewChunk(File* file, string chunkName, int msgSize) {
        ProcessInfo server = randomSelect(allServers);
        Logger("[Creating new file chunk at]: " + server.processID);
        string fileName = getChunkFile(file->name, chunkName);
        connectAndSend(server.processID, "create", "", 2, fileName);
        updateMetaData(file, server, msgSize);
        return server;
    }

    // Updating meta-data in the meta Directory
    void updateMetaData(File* file, ProcessInfo server, int msgSize = 0) {
        server.addFile(getChunkFile(file->name, to_string(file->chunks)));
        file->chunks++;
        file->size += msgSize;
        Logger("[Meta-Data Updated]");
        updateServers(server);
    }

    void updateServers(ProcessInfo server) {
        int index = 0;
        for (ProcessInfo s : allServers) {
            if (s.processID == server.processID) break;
            index++;
        }
        allServers.at(index) = server;
    }

    void registerHeartBeat(Message* m, int index) {
        Logger("[HEARTBEAT] " + m->sourceID);
        allServers[index].setAlive();
        allServers[index].updateFiles(m->message);
    }

    void serversAlive() {
        while (1) {
            for (int i = 0; i < allServers.size(); i++) {
                allServers[i].checkAlive();
            }
        }
    }

    ~Mserver() { updateCsv("csvs/files.csv", files); }
};

// IO for continuous read and write messages
// @client - Process
void io(Mserver* mserver) {
    int rw = 25;
    int i = 0;
    sleep(rw);
    for (ProcessInfo server : mserver->allServers) {
        cout << server.getAlive() << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "usage %s ID port directory_path\n", argv[0]);
        exit(1);
    }

    Mserver* server = new Mserver(argv);

    std::thread listenerThread(&Mserver::listener, server);
    std::thread countDown(&Mserver::serversAlive, server);
    countDown.join();
    listenerThread.join();

    logger.close();
    return 0;
}