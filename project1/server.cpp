/*
    @author: antriksh
    Version 0: 2/5/2018
*/

#include "Message.hpp"

using namespace std;

string logfile;
ofstream logger;

string globalTime() {
    time_t now = time(0);   // get time now
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void Logger(string message, int clock) {
    logger << "[" << globalTime() << "][CLOCK: " << clock << "]::" << message << endl;
    cout << "[" << globalTime() << "][CLOCK: " << clock << "]::" << message << endl;
    logger.flush();
    return;
}

void error(const char *msg, int clock) {
    Logger(msg, clock);
    perror(msg);
    exit(1);
}

class Server {
    /*
        Server:
        * Manages files and file paths
        * Takes client requests to read and write
        * Does not do anything for maintaining mutual exclusion
    */

private:
    int clock = 0;
    string serverID;
    int personalfd;
    int port;
    string serverDirectory;

public:
    int portno;
    // int clients[100], numClients = 0;
    string clientIds[100];
    socklen_t clilen;
    char *buffer;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    queue<Message*> messageQueue;
    vector<ProcessInfo> allClients, allServers;

    Server(char* argv[], vector<ProcessInfo> clients, vector<ProcessInfo> servers) {

        serverID = argv[1];
        logger.open("logs/" + serverID + ".txt", ios::app | ios::out);

        allClients = clients;
        allServers = servers;

        serverDirectory = argv[3];

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
        inet_ntop( AF_INET, &ipAddr, ipAddress, INET_ADDRSTRLEN);

        Logger("Process Address: " + string(ipAddress), this->clock);
        Logger("Process ID: " + string(serverID), this->clock);
        Logger("Process FD: " + to_string(personalfd), this->clock);

        // bind the socket using the bind() system call
        if (::bind(personalfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR on binding", this->clock);
        Logger("Bind complete !", this->clock);

        // listen for connections using the listen() system call
        listen(personalfd, 5);
        clilen = sizeof(cli_addr);
        Logger("Waiting for communication...", this->clock);
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
        // Logger(hostname + " found !", this->clock);

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

        Message *msg = new Message(false, rw, message, source, serverID, destination, this->clock, filename);
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

        if (clientIds[source].empty())
            Logger("[RECEIVED FROM " + to_string(source) + "]: " + message->message, this->clock);
        else
            Logger("[RECEIVED FROM " + clientIds[source] + "]: " + message->message, this->clock);
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
            // Logger("New connection " + to_string(newsockfd), this->clock);

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
                this->checkMessage(message);
            }
        }
    }

    void checkMessage(Message *m) {
        if (m->message == "hi") {
            writeReply(m, "hello");
        } else {
            this->checkReadWrite(m);
        }
    }

    void writeReply(Message *m, string text, int rw=0, string f="NULL") {
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

    void checkReadWrite(Message *m) {
        switch (m->readWrite) {
        case 1: {
            string line = readFile(m->fileName);
            writeReply(m, line, 1, m->fileName);
            break;
        }
        case 2: {
            writeToFile(m->fileName, m->message);
            writeReply(m, m->message, 2, m->fileName);
            break;
        }
        case 3: {
            vector<string> files = this->readDirectory(serverDirectory);
            writeReply(m, this->sendFiles(files), 3);
            break;
        }
        default: {
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

    string sendFiles(vector<string> files) {
        string allfiles = "";
        for (string file: files) {
            allfiles += file + ":";
        }
        cout << allfiles << endl;
        return allfiles.substr(0, allfiles.length() - 1);
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

    ~Server() {
        close(personalfd);
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

    std::thread ProcessThread(&Server::processMessages, server);
    std::thread connectionThread(&Server::acceptConnection, server);

    ProcessThread.join();
    connectionThread.join();

    logger.close();
    return 0;
}