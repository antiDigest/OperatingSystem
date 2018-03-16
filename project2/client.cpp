/*
    @author: antriksh
    Version 0: 3/14/2018 : TODO
*/

#include "header/MetaInfo.hpp"
#include "header/Socket.hpp"

// Process: part of the lamport's mutual exclusion algorithm
// * to perform a read, gets meta-data of file from mServer and
// requests the related Server to read
// * to perform a write, gets meta-data of file from mServer and
// requests the related Server to append the line
// * Has a unique ID (given by the user)
class Process : protected Socket {
   private:
    priority_queue<Message *> readRequestQueue;
    priority_queue<Message *> writeRequestQueue;
    bool pendingEnquiry = false, pendingRead = false, pendingWrite = false;
    bool waitForWriteRelease = false, waitForReadRelease = false;
    int readReplyCount, writeReplyCount;
    Message *pendingReadMessage;
    string pendingWriteMessage;

   public:
    // Enquires about the files in the system
    // sends requests for read and write of files to server while
    // maintaining mutual exclusion
    Process(char *argv[]) : Socket(argv) { this->enquiry(); }

    // Infinite thread to accept connection and detach a thread as
    // a receiver and checker of messages
    void listener() {
        while (1) {
            // Accept a connection with the accept() system call
            int newsockfd =
                accept(personalfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd < 0) {
                error("ERROR on accept");
            }
            Logger("New connection " + to_string(newsockfd));

            std::thread connectedThread(&Process::processMessages, this,
                                        newsockfd);
            connectedThread.detach();
        }
    }

    // Starts as a thread which receives a message and checks the message
    // @newsockfd - fd socket stream from which message would be received
    void processMessages(int newsockfd) {
        try {
            Message *message = this->receive(newsockfd);
            close(newsockfd);
            this->checkMessage(message, newsockfd);
        } catch (const char *e) {
            Logger(e);
            // break;
        }
    }

    // Checks the message for different types of incoming messages
    // 1. hi - just a hello from some client/server (not in use)
    // 2. meta - reply from the meta-server with resource meta-data
    // 3. reply - reply from a server to a request
    // 4. FAILED - failed response to a request
    // @m - Message just received
    // @newsockfd - socket stream it was received from
    void checkMessage(Message *m, int newsockfd) {
        if (m->type == "hi") {
            // connectAndReply(m, newsockfd, "hello");
            throw "BREAKING CONNECTION";
        }
        // meta-data from the m-server
        else if (m->type == "meta") {
            // TODO: process response from meta-server
            MetaInfo *meta = stringToInfo(m->message);
            m->fileName = meta->getChunkFile();
            ProcessInfo server = findInVector(allServers, meta->server);
            criticalSection(m, server);
        }
        // reply message from a server for read
        else if (m->type == "reply" && m->readWrite == 1) {
            // TODO: read response from a server
        }
        // reply message from a server for write
        else if (m->type == "reply" && m->readWrite == 2) {
            // TODO: write response from a server
        }
        // Failed response from meta-server
        else if (m->type == "FAILED") {
            // TODO: write response from a server
            if (m->readWrite == 1) {
                pendingRead = false;
            } else {
                pendingWrite = false;
            }
        }
        // Enquiry response
        else if (m->type == "enquiry") {
            // TODO: write response from a server
            allFiles = getFiles(m->message);
        }

        throw "BREAKING CONNECTION";
    }

    // Enuiry - message to the meta-server for a list of files
    void enquiry() {
        ProcessInfo server = mserver[0];
        while (1) {
            try {
                Logger("Connecting to " + server.processID + " at " +
                       server.hostname);
                int fd = this->connectTo(server.hostname, server.port);
                this->send(personalfd, fd, "enquiry", "", server.processID, 3);
                close(fd);
                break;
            } catch (const char *e) {
                Logger(e);
            }
        }
    }

    // gets MetaData from the meta-server
    // requests the meta-server returned server for the resource
    void readRequest() {
        string fileName = "file1";
        int rw = 1;
        string message = "request";

        getMetaData(rw, message, fileName);
    }

    // gets MetaData from the meta-server
    // requests the meta-server returned server for the resource
    void writeRequest() {
        string fileName = "file1";
        int rw = 2;
        string message =
            "This is a random line I would like to write to a file";

        getMetaData(rw, message, fileName);
    }

    // Section where the client corresponds with the server for read/write
    // @m - Message that was initially sent for request
    void criticalSection(Message *m, ProcessInfo server) {
        string cs = "[CRITICAL SECTION]";
        Logger(cs);

        int fd = -1;
        while (fd < 0) {
            try {
                fd = connectTo(server.hostname, server.port);
            } catch (const char *e) {
                Logger(cs + e);
            }
        }

        // TODO: Wasn't sending last I checked
        if (m->readWrite == 1) {
            this->send(this->personalfd, fd, "request", "", server.processID,
                       m->readWrite, m->fileName);
            Message *msg = this->receive(fd);
            Logger(cs + "[READ]" + m->fileName + "[LINE]" + m->message);
        } else if (m->readWrite == 2) {
            this->send(this->personalfd, fd, "request", pendingWriteMessage,
                       server.processID, m->readWrite, m->fileName);
            Message *msg = this->receive(fd);
            Logger(cs + "[WRITE]" + m->fileName + "[LINE]" + m->message);
        }

        Logger(cs + "[EXIT]");
    }

    // Get Meta Data from the Meta-Server, send the meta-server a request for
    // meta-data
    // @rw - read/write request
    // @message - Message (for read, it is empty, for write it is the sentence
    // you want to write)
    // @fileName - file you would like to write to
    void getMetaData(int rw, string message, string fileName) {
        ProcessInfo p = mserver[0];
        int fd = this->connectTo(p.hostname, p.port);
        send(personalfd, fd, "request", message, p.processID, rw, fileName);
        if (rw == 1) {
            pendingRead = true;
        } else {
            pendingWrite = true;
            pendingWriteMessage = message;
        }
        close(fd);
    }
};

// IO for continuous read and write messages
// @client - Process
void io(Process *client) {
    int rw = rand() % 10;
    cin >> rw;
    int i = 0;
    while (i < rw) {
        sleep(rw);
        client->writeRequest();
        sleep(rw);
        client->readRequest();
        i++;
    }
}

// IO for continuous enquiry messages
// @client - Process
void eio(Process *client) {
    int rw = rand() % 1;
    int i = 0;
    while (i < 3) {
        sleep(2);
        client->enquiry();
        // sleep(2);
        // client->writeRequest();
        i++;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage %s ID port", argv[0]);
        exit(0);
    }

    // So that others can be activated
    // sleep(5);
    Process *client = new Process(argv);

    std::thread listenerThread(&Process::listener, client);
    io(client);
    listenerThread.join();

    logger.close();
    return 0;
}