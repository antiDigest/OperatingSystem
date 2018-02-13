// Socket.hpp
/*
    @author: antriksh
    Version 0: 2/12/2018
*/

#include "Message.hpp"
#include "utils.hpp"
#include "ProcessInfo.hpp"

class Socket {

protected:
    int clock = 0;
    string id;
    int personalfd;
    int port;
    string directory;

public:
    int portno;
    socklen_t clilen;
    char *buffer;
    struct hostent *server;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    queue<Message*> messageQueue;
    vector<ProcessInfo> allClients, allServers;
    vector<string> allFiles;
    string others[100];
    priority_queue<Message> readRequestQueue;
    priority_queue<Message> writeRequestQueue;

    Socket(char* argv[], vector<ProcessInfo> clients, vector<ProcessInfo> servers) {

        allClients = clients;
        allServers = servers;

        portno = atoi(argv[2]);

        // Create a socket using socket() system call
        personalfd = socket(AF_INET, SOCK_STREAM, 0);
        if (personalfd < 0)
            error("ERROR opening socket", this->clock);
        Logger("Socket connection established !", this->clock);

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);

        struct in_addr ipAddr = serv_addr.sin_addr;
        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ipAddr, ipAddress, INET_ADDRSTRLEN);

        Logger("Process Address: " + string(ipAddress), this->clock);
        Logger("Process ID: " + string(id), this->clock);
        Logger("Process FD: " + to_string(personalfd), this->clock);

        // bind the socket using the bind() system call
        if (::bind(personalfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR on binding", this->clock);
        Logger("Bind complete !", this->clock);

        // listen for connections using the listen() system call
        listen(personalfd, 5);
        clilen = sizeof(cli_addr);
        Logger("Listening for connections...", this->clock);

	    std::thread connectionThread(&Socket::acceptConnection, this);
	    std::thread ProcessThread(&Socket::processMessages, this);

	    connectionThread.join();
	    ProcessThread.join();
    }

    void sayHello() {
        for (ProcessInfo server: allServers) {
            try {
                if (server.processID != this->id) {
                    Logger("Connecting to " + server.processID + " at " + server.hostname, this->clock);
                    int fd = this->connectTo(server.hostname, server.port);
                    this->send(personalfd, fd, "hi", server.processID);
                    // Message *m = this->receive(fd);
                }
            }
            catch (const char* e) {
                Logger(e, this->clock);
                continue;
            }
        }
        for (ProcessInfo client: allClients) {
            try {
                Logger("Connecting to " + client.processID + " at " + client.hostname, this->clock);
                int fd = this->connectTo(client.hostname, client.port);
                this->send(personalfd, fd, "hi", client.processID);
                // Message *m = this->receive(fd);
            }
            catch (const char* e) {
                Logger(e, this->clock);
                continue;
            }
        }
    }

    int connectTo(string hostname, int portno) {

        // Create a socket using socket() system call
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            throw "ERROR opening socket";

        // Search for host
        struct hostent *host = gethostbyname(hostname.c_str());
        if (host == NULL) {
            throw "ERROR, no such host";
        }
        Logger(hostname + " found !", this->clock);

        struct sockaddr_in host_addr, cli_addr;
        bzero((char *) &host_addr, sizeof(host_addr));
        host_addr.sin_family = AF_INET;
        bcopy((char *)host->h_addr,
              (char *)&host_addr.sin_addr.s_addr,
              host->h_length);
        host_addr.sin_port = htons(portno);

        // Attempting to connect to host
        if (connect(fd, (struct sockaddr *) &host_addr, sizeof(host_addr)) < 0)
            throw "ERROR connecting, the host may not be active";

        return fd;
    }

    void send(int source, int destination, string message, string destID, int rw=0, string filename="NULL") {

        Message *msg = new Message(false, rw, message, source, id, destination, this->clock, filename);
        this->setClock();

        n = write(destination, messageString(msg).c_str(), 1024);
        if (n < 0)
            error("ERROR writing to socket", this->clock);

        Logger("[SENT TO " + destID + "]: " + message, this->clock);
    }

    Message *receive(int source) {
        char msg[1024];
        bzero(msg, 1024);
        n = read(source, msg, 1024);
        if (n < 0) {
            error("ERROR reading from socket", this->clock);
        }

        Message *message = getMessage(msg);
        this->setClock(message->timestamp);

        Logger("[RECEIVED FROM " + message->sourceID + "]: " + message->message, this->clock);
        return message;
    }

    void acceptConnection() {
        while (1) {
            // Accept a connection with the accept() system call
            // cout << messageQueue.front() << endl;
            int newsockfd = accept(personalfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0) {
                // continue;
                error("ERROR on accept", this->clock);
            }
            Logger("New connection " + to_string(newsockfd), this->clock);

            // this->introduce(newsockfd);
            Message* message = this->receive(newsockfd);
            messageQueue.push(message);

            close(newsockfd);
        }
    }

    void processMessages() {
        Message *message;
        while (1) {
            if (!messageQueue.empty()) {
                message = messageQueue.front();
                messageQueue.pop();
                // this->checkMessage(message);
            }
        }
    }

    void writeMessage(Message *m, string text, int rw, string f="NULL"){
        ProcessInfo client = getFd(m);
        try {
            int fd = connectTo(client.hostname, client.port);
            this->send(m->destination, fd, text, client.processID, rw, f);
            close(fd);
        } catch (const char* e) {
            Logger(e, this->clock);
            return;
        }
    }

    ProcessInfo getFd(Message *m) {
        string sid = m->sourceID;
        try {
            ProcessInfo client = findInVector(allClients, sid);
            return client;
        } catch (const char* e) {}
        try {
            ProcessInfo server = findInVector(allServers, sid);
            return server;
        } catch (const char* e) {
            throw e;
        }
    }

    void setClock() {
        this->clock += D;
    }

    void setClock(int timestamp) {
        this->clock = max(this->clock + D, timestamp);
    }

    void resetClock() {
        this->clock = 0;
    }

    ~Socket() {
        close(personalfd);
    }
};