/*
    @author: antriksh
    Version 0: 2/5/2018
    Version 1: 2/14/2018: 
                * First Successful run !
                * Documentation improved
                * all critical sections are being visited
                * all read and write requests are being handled
                * all files are updated (on all the servers)
                * Haven't tested it for more than 3 servers and 5 clients
                * Haven't tested it with servers and clients being on different systems
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
            Enquires about the files in the system
            sends requests for read and write of files to server while maintaining mutual exclusion
        */
        this->enquiry();
    }

    void listener() {
        /*
            Infinite thread to accept connection and detach a thread as
            a receiver and checker of messages
        */
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

    void processMessages(int newsockfd) {
        /*
            Starts as a thread which receives a message and checks the message
            @newsockfd - fd socket stream from which message would be received
        */
        try {
            Message* message = this->receive(newsockfd);
            close(newsockfd);
            this->checkMessage(message, newsockfd);
        } catch (const char* e) {
            Logger(e, this->clock);
            // break;
        }
    }

    void processReadWrite(){
        /*
            Checks if the process (client) is ready to visit the criticalSection and release the resource
            Also re-initializes everything
        */
        // Am I allowed to read ?
        if (pendingRead && readReplyCount == allClients.size() - 1 
            && !waitForReadRelease && readRequestQueue.top()->sourceID == this->id) {
            Logger("[ALL REPLIED/RELEASED, MOVING IN]", this->clock);
            criticalSection(pendingReadMessage);
            sendRelease(pendingReadMessage);
            readReplyCount = 0;
            pendingRead = false;
        }
        // Am I allowed to write ?
        else if (pendingWrite && writeReplyCount == allClients.size() - 1
            && !waitForWriteRelease && writeRequestQueue.top()->sourceID == this->id) {
            Logger("[ALL REPLIED/RELEASED, MOVING IN]", this->clock);
            criticalSection(pendingWriteMessage);
            sendRelease(pendingWriteMessage);
            writeReplyCount = 0;
            pendingWrite = false;
        }
    }

    void checkMessage(Message *m, int newsockfd) {
        /*
            Checks the message for different types of incoming messages
            1. hi - just a hello from some client/server (not in use)
            2. request - request to access (read/write) a resource
            3. reply - reply to a request
            4. release - after using a resource, message identifying job done
            ends with a check on if the process is ready to enter the criticalSection
            @m - Message just received
            @newsockfd - socket stream it was received from
        */
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
        /*
            Enuiry - message to all servers for a list of files
        */
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
        /*
            Attempt to request a resource (file) for read
            sends the request to all other clients connected
            updates its parameters to be aware of request made
        */
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
        /*
            Attempt to request a resource (file) for write
            sends the request to all other clients connected
            updates its parameters to be aware of request made
        */
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
        /*
            Updates all other clients that the resource requested has been accessed
            and job is done
        */
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
        /*
            Section where the client corresponds with the server for read/write
            @m - Message that was initially sent for request
        */
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
    /*
        IO for continuous read and write messages
        @client - Process
    */
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
    /*
        IO for continuous enquiry messages
        @client - Process
    */
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
        exit(0);
    }

    // So that others can be activated
    sleep(5);
    Process *client = new Process(argv);

    std::thread listenerThread(&Process::listener, client);
    io(client);
    listenerThread.join();

    logger.close();
    return 0;
}