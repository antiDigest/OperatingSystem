/*
    @author: antriksh
    Version 0: 2/5/2018
*/

#include "header/Socket.hpp"

class Process: protected Socket {

    /*
        Process: part of the lamport's mutual exclusion algorithm
        * connects to all servers and stores a list of all other clients
        * maintains mutual exclusion while attempting to read and write
        * Has a clock (Lamport's clock)
        * Has a unique ID (given by the user)
    */

private:
    priority_queue<Message*> readRequestQueue;
    priority_queue<Message*> writeRequestQueue;
    bool pendingEnquiry=false, pendingRead=false, pendingWrite=false;
    bool waitForWriteRelease=false, waitForReadRelease=false;
    int replyCount;
    Message* pendingReadMessage, pendingWriteMessage;

public:
    Process(char *argv[]): Socket(argv) {
        /*
            Initiates a socket connection
            Finds the port(s) to connect to for the servers
            initializes the list of clients
            sends requests for read and write of files to server while maintaining mutual exclusion
        */
        // cout << "Sending enquiry !" << endl;
        this->enquiry();
    }

    void listener() {
        while (1) {
            // Accept a connection with the accept() system call
            int newsockfd = accept(personalfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0) {
                error("ERROR on accept", this->clock);
            }
            Logger("New connection " + to_string(newsockfd), this->clock);

            // this->spawnNewThread(newsockfd);
            std::thread connectedThread(&Process::processMessages, this, newsockfd);
            connectedThread.detach();
        }
    }

    void spawnNewThread(int newsockfd) {
        std::thread connectedThread(&Process::processMessages, this, newsockfd);
        connectedThread.join();
        close(newsockfd);
    }

    void processMessages(int newsockfd) {
        while (1) {
            try {
                Message* message = this->receive(newsockfd);
                this->checkMessage(message, newsockfd);
            } catch (const char* e) {
                Logger(e, this->clock);
                close(newsockfd);
                break;
            }
        }
    }

    void checkMessage(Message *m, int newsockfd) {
        if (m->message == "hi") {
            writeReply(m, newsockfd, "hello");
            throw "BREAKING CONNECTION";
        }
        // request from a client
        else if (m->message == "request") {
            if (m->readWrite == 2) {
                Logger("[WRITE REQUEST FOR FILE]: " + m->fileName, this->clock);
                if (pendingWrite && pendingWriteMessage->fileName == m->fileName) {
                    writeRequestQueue.push(m);
                    Message *write = writeRequestQueue.top();
                    if (write->sourceID != this->id) {
                        writeReply(m, newsockfd, "reply");
                        waitForWriteRelease = true;
                    } else {
                        criticalSection(m);
                        sendRelease(m);
                        throw "BREAKING CONNECTION";
                    }
                } else {
                    writeReply(m, newsockfd, "reply");
                }
            } else if (m->readWrite == 1) {
                Logger("[READ REQUEST FOR FILE]: " + m->fileName, this->clock);
                if (pendingRead && pendingReadMessage->fileName == m->fileName) {
                    readRequestQueue.push(m);
                    Message *write = readRequestQueue.top();
                    if (write->sourceID != this->id) {
                        writeReply(m, newsockfd, "reply");
                        waitForReadRelease = true;
                    } else {
                        criticalSection(m);
                        sendRelease(m);
                        throw "BREAKING CONNECTION";
                    }
                } else {
                    writeReply(m, newsockfd, "reply");
                }
            }
        }
        // reply message from a client
        else if (m->message == "reply") {
            for (ProcessInfo client: allClients) {
                if (m->sourceID == client.processID) {
                    client.replied = true;
                }
                if (client.replied == true)
                    replyCount++;
            }
            if (replyCount == allClients.size()) {
                criticalSection(m);
                sendRelease(m);
                throw "BREAKING CONNECTION";
            }
        }
        // release message
        else if (m->message == "release") {
            if (m->readWrite == 2) {
                Logger("[WRITE RELEASE FOR FILE]: " + m->fileName, this->clock);
                writeRequestQueue.pop();
                if (waitForRelease) {
                    Message *read = writeRequestQueue.top();
                    if (read->sourceID != this->id) {
                        writeReply(m, newsockfd, "reply");
                    } else {
                        criticalSection(m);
                        sendRelease(m);
                        throw "BREAKING CONNECTION";
                    }
                }
            } else if (m->readWrite == 1) {
                Logger("[READ RELEASE FOR FILE]: " + m->fileName, this->clock);
                readRequestQueue.pop();
                if (waitForRelease) {
                    Message *read = readRequestQueue.top();
                    if (read->sourceID != this->id) {
                        writeReply(m, newsockfd, "reply");
                    } else {
                        criticalSection(m);
                        sendRelease(m);
                        throw "BREAKING CONNECTION";
                    }
                }
            }
        }
        throw "BREAKING CONNECTION";
    }

    void enquiry() {
        for (ProcessInfo server: allServers) {
            try {
                Logger("Connecting to " + server.processID + " at " + server.hostname, this->clock);
                int fd = this->connectTo(server.hostname, server.port);
                this->send(personalfd, fd, "enquiry", server.processID, 3);
                Message *m = this->receive(fd);
                allFiles = getFiles(m->message);
                close(fd);
                break;
            }
            catch (const char* e) {
                Logger(e, this->clock);
                continue;
            }
        }
        pendingEnquiry = true;
    }

    void readRequest() {
        for (ProcessInfo client: allClients) {
            try {
                if (client.processID != this->id) {
                    Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                    int fd = this->connectTo(client.hostname, client.port);
                    Message* m = this->send(personalfd, fd, "request", client.processID, 1, "file1");
                    pendingReadMessage = m;
                    readRequestQueue.push(m);
                    pendingRead = true;
                    // Message *m = this->receive(fd);
                }
            }
            catch (const char* e) {
                Logger(e, this->clock);
                continue;
            }
        }
    }

    void writeRequest() {
        for (ProcessInfo client: allClients) {
            try {
                if (client.processID != this->id) {
                    Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                    int fd = this->connectTo(client.hostname, client.port);
                    Message* m = this->send(personalfd, fd, "request", client.processID, 2, "file1");
                    pendingWriteMessage = m;
                    writeRequestQueue.push(m);
                    pendingWrite = true;
                    // Message *m = this->receive(fd);
                }
            }
            catch (const char* e) {
                Logger(e, this->clock);
                continue;
            }
        }
    }

    void sendRelease(Message *m) {
        for (ProcessInfo client: allClients) {
            try {
                if (client.processID != this->id) {
                    Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                    int fd = this->connectTo(client.hostname, client.port);
                    Message* m = this->send(personalfd, fd, "release", client.processID, m->readWrite, m->fileName);
                    readRequestQueue.pop();
                    if (m->readWrite == 1) {
                        pendingRead = false;
                    } else if (m->readWrite == 2) {
                        pendingWrite = false;
                    } else {
                        pendingEnquiry = false;
                    }
                    client.replied = false;
                    // Message *m = this->receive(fd);
                }
            }
            catch (const char* e) {
                Logger(e, this->clock);
                continue;
            }
        }
    }

    void criticalSection(Message *m) {
        string cs = "[CRITICAL SECTION]";
        Logger(cs, this->clock);

        for(ProcessInfo server: allServers){
            int fd = -1;
            while(fd < 0){
                try{
                    fd = connectTo(server.hostname, server.port);
                } catch(const char* e) {
                    Logger(cs + e, this->clock);
                }
            }
            this->send(this->personalfd, fd, "request", server.processID, m->readWrite, m->fileName);
            Message *m = this->receive(fd);
            Logger(cs + "[READ]" + m->fileName + "[LINE]" + m->message, this->clock);
        }

        Logger(cs + "[EXIT]", this->clock);
    }

};

void io(Process *client) {
    int rw = rand() % 1;
    int i=0;
    while (i < 10) {
        sleep(5);
        client->readRequest();
        sleep(5);
        client->writeRequest();
        i++;
    }
}

void eio(Process* client) {
    int rw = rand() % 1;
    int i=0;
    while (i < 3) {
        sleep(2);
        client->enquiry();
        // sleep(2);
        // client->writeRequest();
        i++;
    }
}

int main(int argc, char *argv[])
{


    if (argc < 3) {
        fprintf(stderr, "usage %s ID port", argv[0]);
        // Logger("usage " + string(argv[0]) + " ID hostname port");
        exit(0);
    }

    sleep(10);
    Process *client = new Process(argv);
    // io(client);

    std::thread listenerThread(&Process::listener, client);
    io(client);
    listenerThread.join();

    logger.close();
    return 0;
}