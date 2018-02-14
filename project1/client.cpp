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
    int readReplyCount, writeReplyCount;
    Message* pendingReadMessage;
    Message* pendingWriteMessage;

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

            std::thread connectedThread(&Process::processMessages, this, newsockfd);
            connectedThread.detach();
        }
    }

    void spawnNewThread(int newsockfd) {
        
    }

    void processMessages(int newsockfd) {
        while (1) {
            try {
                Message* message = this->receive(newsockfd);
                close(newsockfd);
                this->checkMessage(message, newsockfd);
            } catch (const char* e) {
                Logger(e, this->clock);
                break;
            }
        }
    }

    void processReadWrite(){
        // while(1){
        // Am I allowed to read ?
        if (pendingRead && readReplyCount == allClients.size() - 1 
            && !waitForReadRelease && readRequestQueue.top()->sourceID == this->id) {
            Logger("[ALL REPLIED/RELEASED, MOVING IN]", this->clock);
            criticalSection(pendingReadMessage);
            sendRelease(pendingReadMessage);
            readReplyCount = 0;
            pendingRead = false;
            // throw "BREAKING CONNECTION";
        }
        // Am I allowed to write ?
        else if (pendingWrite && writeReplyCount == allClients.size() - 1
            && !waitForWriteRelease && writeRequestQueue.top()->sourceID == this->id) {
            Logger("[ALL REPLIED/RELEASED, MOVING IN]", this->clock);
            criticalSection(pendingWriteMessage);
            sendRelease(pendingWriteMessage);
            writeReplyCount = 0;
            pendingWrite = false;
            // throw "BREAKING CONNECTION";
        }
        // }
    }

    void checkMessage(Message *m, int newsockfd) {
        if (m->message == "hi") {
            connectAndReply(m, newsockfd, "hello");
            throw "BREAKING CONNECTION";
        }
        // request from a client
        else if (m->message == "request") {
            if (m->readWrite == 2) {
                Logger("[WRITE REQUEST FOR FILE]: " + m->fileName, this->clock);
                connectAndReply(m, newsockfd, "reply");
                if (pendingWrite && pendingWriteMessage->fileName == m->fileName) {
                    Logger("[MY WRITE REQUEST FOR FILE]: " + pendingWriteMessage->fileName, this->clock);
                    writeRequestQueue.push(m);
                    Message *write = writeRequestQueue.top();
                    if (write->sourceID != this->id) {
                        waitForWriteRelease = true;
                    }
                }
            } else if (m->readWrite == 1) {
                Logger("[READ REQUEST FOR FILE]: " + m->fileName, this->clock);
                connectAndReply(m, newsockfd, "reply");
                if (pendingRead && pendingReadMessage->fileName == m->fileName) {
                    Logger("[MY READ REQUEST FOR FILE]: " + pendingReadMessage->fileName, this->clock);
                    readRequestQueue.push(m);
                    Message *write = readRequestQueue.top();
                    if (write->sourceID != this->id) {
                        waitForReadRelease = true;
                    }
                }
            }
        }
        // reply message from a client for read
        else if (m->message == "reply" && m->readWrite == 1) {
            for (ProcessInfo client: allClients) {
                if (m->sourceID == client.processID) {
                    client.repliedRead = true;
                }
                if (client.repliedRead == true)
                    readReplyCount++;
            }
        }
        // reply message from a client for write
        else if (m->message == "reply" && m->readWrite == 2) {
            for (ProcessInfo client: allClients) {
                if (m->sourceID == client.processID) {
                    client.repliedWrite = true;
                }
                if (client.repliedWrite == true)
                    writeReplyCount++;
            }
        }
        // release message
        else if (m->message == "release") {
            if (m->readWrite == 2) {
                Logger("[WRITE RELEASE FOR FILE]: " + m->fileName, this->clock);
                if (waitForWriteRelease){
                    writeRequestQueue.pop();
                    if (writeRequestQueue.top()->sourceID == this->id)
                        waitForWriteRelease = false;
                    else
                        waitForWriteRelease = true;
                }
            } else if (m->readWrite == 1) {
                Logger("[READ RELEASE FOR FILE]: " + m->fileName, this->clock);
                if (waitForReadRelease){
                    readRequestQueue.pop();
                    if(readRequestQueue.top()->sourceID == this->id)
                        waitForReadRelease = false;
                    else
                        waitForReadRelease = true;
                }
            }
        }

        processReadWrite();

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
        Message *m = new Message(false, 1, "request", personalfd, this->id, -1, this->clock, randomFileSelect(allFiles));
        for (ProcessInfo client: allClients) {
            try {
                if (client.processID != this->id) {
                    Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                    int fd = this->connectTo(client.hostname, client.port);
                    this->send(m, fd, client.processID);
                    close(fd);
                }
            }
            catch (const char* e) {
                Logger(e, this->clock);
                continue;
            }
        }
        pendingReadMessage = m;
        readRequestQueue.push(m);
        pendingRead = true;
        // std::thread connectedThread(&Process::processReadWrite, this);
        // connectedThread.detach();
    }

    void writeRequest() {
        Message *m = new Message(false, 2, "request", personalfd, this->id, -1, this->clock, randomFileSelect(allFiles));
        for (ProcessInfo client: allClients) {
            try {
                if (client.processID != this->id) {
                    Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                    int fd = this->connectTo(client.hostname, client.port);
                    this->send(m, fd, client.processID);
                    close(fd);
                }
            }
            catch (const char* e) {
                Logger(e, this->clock);
                continue;
            }
        }
        pendingWriteMessage = m;
        writeRequestQueue.push(m);
        pendingWrite = true;
        // std::thread connectedThread(&Process::processReadWrite, this);
        // connectedThread.detach();
    }

    void sendRelease(Message *m) {
        if (m->readWrite == 1)
            readRequestQueue.pop();
        else if (m->readWrite == 2)
            writeRequestQueue.pop();
        for (ProcessInfo client: allClients) {
            try {
                if (client.processID != this->id) {
                    Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                    int fd = -1;
                    while(fd < 0){
                        try{
                            fd = connectTo(client.hostname, client.port);
                        } catch(const char* e) {
                            Logger(e, this->clock);
                        }
                    }
                    this->send(personalfd, fd, "release", client.processID, m->readWrite, m->fileName);
                    if (m->readWrite == 1) {
                        client.repliedRead = false;
                    } else if (m->readWrite == 2) {
                        client.repliedWrite = false;
                    }
                    close(fd);
                }
            } catch (const char* e) {
                Logger(string(e) + "a", this->clock);
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
            if (m->readWrite == 1)
                Logger(cs + "[READ]" + m->fileName + "[LINE]" + m->message, this->clock);
            else if (m->readWrite == 2)
                Logger(cs + "[WRITE]" + m->fileName + "[LINE]" + m->message, this->clock);
        }

        Logger(cs + "[EXIT]", this->clock);
    }

};

void io(Process *client) {
    int rw = rand() % 10;
    cin >> rw;
    int i=0;
    while (i < rw) {
        sleep(rw);
        client->writeRequest();
        sleep(rw);
        client->readRequest();
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

    sleep(5);
    Process *client = new Process(argv);
    // io(client);

    std::thread listenerThread(&Process::listener, client);
    io(client);
    listenerThread.join();

    logger.close();
    return 0;
}