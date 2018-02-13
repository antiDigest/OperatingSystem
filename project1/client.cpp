/*
    @author: antriksh
    Version 0: 2/5/2018
*/

#include "header/Socket.hpp"

class Process {

    /*
        Process: part of the lamport's mutual exclusion algorithm
        * connects to all servers and stores a list of all other clients
        * maintains mutual exclusion while attempting to read and write
        * Has a clock (Lamport's clock)
        * Has a unique ID (given by the user)
    */

private:
    priority_queue<Message> readRequestQueue;
    priority_queue<Message> writeRequestQueue;
    bool pendingEnquiry=false, pendingRead=false, pendingWrite=false, waitForRelease=false;
    int replyCount;

public:
    Process(char *argv[], vector<ProcessInfo> clients, vector<ProcessInfo> servers)
    : Socket(argv, clients, servers) {
        /*
            Initiates a socket connection
            Finds the port(s) to connect to for the servers
            initializes the list of clients
            sends requests for read and write of files to server while maintaining mutual exclusion
        */

        id = argv[1];
        logfile = processId;
        logger.open("logs/" + logfile + ".txt", ios::app | ios::out);

        // this->sayHello();
        this->enquiry();
    }

    void checkMessage(Message *m) {
        if (m->message == "hi") {
            // otherFds[numBeings++] = m->source;
            // others[m->source] = m->sourceID;
            writeReply(m, "hello");
        } else if (m->message == "hello") {
            // if(others[m->source].empty()){
            //     otherFds[numBeings++] = m->source;
            //     others[m->source] = m->sourceID;
            // }
        } else if (m->message == "request") {
            if (m->readWrite == 2) {
                Logger("[WRITE REQUEST FOR FILE]: " + m->fileName, this->clock);
                writeRequestQueue.push(m);
                if (pendingRead){
                    Message *write = writeRequestQueue.top();
                    if(write->sourceID != processId){
                        writeReply(m, "reply");
                    }
                }
            } else if (m->readWrite == 1) {
                Logger("[READ REQUEST FOR FILE]: " + m->fileName, this->clock);
                readRequestQueue.push(m);
                if (pendingRead){
                    Message *read = readRequestQueue.top();
                    if(read->sourceID != processId){
                        writeReply(m, "reply");
                    }
                }
            }
        } else if (m->message == "reply") {
            replyCount++;
            if(replyCount == allClients.size()){
                criticalSection(m);
                sendRelease(m);
            }
        } else if (m->message == "release") {
            if (m->readWrite == 2) {
                Logger("[WRITE RELEASE FOR FILE]: " + m->fileName, this->clock);
                writeRequestQueue.pop();
                if(waitForRelease){
                    Message *read = readRequestQueue.top();
                    if(read->sourceID != processId){
                        writeReply(m, "reply");
                    }
                }
            } else if (m->readWrite == 1) {
                Logger("[READ RELEASE FOR FILE]: " + m->fileName, this->clock);
                readRequestQueue.pop();
                if(waitForRelease){
                    Message *read = readRequestQueue.top();
                    if(read->sourceID != processId){
                        writeReply(m, "reply");
                    }
                }
            }
        } else if (m->message == "") {
        } else if (m->readWrite == 2) {
            Logger("[WRITTEN TO FILE]: " + m->message, this->clock);
        } else if (m->readWrite == 1) {
            Logger("[READ LINE]: " + m->message, this->clock);
        } else if (m->readWrite == 3) {
            Logger("[ALL FILES RECEIVED]: " + m->message, this->clock);
            istringstream line_stream(m->message);
            string token;
            while (getline(line_stream, token, ':')) {
                allFiles.push_back(token);
            }
        }
        return;
    }

    void enquiry() {
        for (ProcessInfo server: allServers) {
            try {
                Logger("Connecting to " + server.processID + " at " + server.hostname, this->clock);
                int fd = this->connectTo(server.hostname, server.port);
                this->send(personalfd, fd, "enquiry", server.processID, 3);
                // Message *m = this->receive(fd);
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
                if (client.processID != processId) {
                    Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                    int fd = this->connectTo(client.hostname, client.port);
                    Message* m = this->send(personalfd, fd, "request", client.processID, 1, "file1");
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
                if (client.processID != processId) {
                    Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                    int fd = this->connectTo(client.hostname, client.port);
                    Message* m = this->send(personalfd, fd, "request", client.processID, 2, "file1");
                    readRequestQueue.push(m);
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
                if (client.processID != processId) {
                    Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                    int fd = this->connectTo(client.hostname, client.port);
                    Message* m = this->send(personalfd, fd, "release", client.processID, m->readWrite, m->fileName);
                    readRequestQueue.pop();
                    if(m->readWrite == 1){
                        pendingRead = false;
                    } else if (m->readWrite == 2){
                        pendingWrite = false;
                    } else {
                        pendingEnquiry = false;
                    }
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
        Logger("[CRITICAL SECTION]", this->clock);
    }

};

void io(Process *client) {
    int rw = rand() % 1;
    int i=0;
    while(i < 10){
        sleep(2);
        client->readRequest();
        sleep(2);
        client->writeRequest();
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

    vector<ProcessInfo> allClients, allServers;
    allClients = readClients(allClients, "clients.csv");
    allServers = readClients(allServers, "servers.csv");
    // sleep(30);
    Process *client = new Process(argv, allClients, allServers);
    // io(client);

    logger.close();
    return 0;
}