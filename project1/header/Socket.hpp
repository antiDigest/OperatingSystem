// Socket.hpp
/*
    @author: antriksh
    Version 0: 2/12/2018
    Version 1: 2/14/2018: 
                * First Successful run !
                * Documentation improved
                * Additional methods for send
                * updated setClock(timestamp)
*/

#include "Message.hpp"
#include "utils.hpp"
#include "ProcessInfo.hpp"

class Socket {
	/*
        Socket:
        * Parent class to Server and Client
        * Manages socket connections, send and receive
        * stores general information of files, clients and servers
    */

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

    Socket(char* argv[]) {
    	/*
            Initiates a socket connection
            Finds the port(s) to connect to for the servers
            initializes the list of clients and servers
        */

        allClients = readClients(allClients, "csvs/clients.csv");
        allServers = readClients(allServers, "csvs/servers.csv");

        id = argv[1];
        logger.open("logs/" + this->id + ".txt", ios::out);

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
    }

    void sayHello() {
    	/*
            Testing method to check if messages sent are being received
        */
        for (ProcessInfo server: allServers) {
            try {
                if (server.processID != this->id) {
                    Logger("Connecting to " + server.processID + " at " + server.hostname, this->clock);
                    int fd = this->connectTo(server.hostname, server.port);
                    this->send(personalfd, fd, "hi", server.processID);
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
            }
            catch (const char* e) {
                Logger(e, this->clock);
                continue;
            }
        }
    }

    int connectTo(string hostname, int portno) {
    	/*
            Connects to a hostname (ip address) at portno (port)
            returns - file descriptor of socket stream connected to
        */

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

    Message* send(int source, int destination, string message, string destID, int rw=0, string filename="NULL") {
    	/*
            Send a message and log the send. Also updates system clock
            @source - source file descriptor
            @destination - destination file descriptor
            @message - string to send
            @destID - ID of the receiver (only for logging)
            @rw - read/write/enquiry (default: none -- 0)
            @filename - on request, the file to read/write (default: "NULL")
			returns - Message sent
        */

        Message *msg = new Message(false, rw, message, source, id, destination, this->clock, filename);
        this->setClock();

        n = write(destination, messageString(msg).c_str(), 1024);
        if (n < 0)
            error("ERROR writing to socket", this->clock);

        Logger("[SENT TO " + destID + "]: " + message, this->clock);

        return msg;
    }

    void send(Message* m, int fd, string destID){
    	/*
            Send a message and log the send
            @m - already compiled Message
            @fd - destination file descriptor
            @destID - ID of the receiver (only for logging)

        */
    	n = write(fd, messageString(m).c_str(), 1024);
        if (n < 0)
            error("ERROR writing to socket", this->clock);

        Logger("[SENT TO " + destID + "]: " + m->message, this->clock);
    }

    Message *receive(int source) {
    	/*
            Receive a message from the source (fd) and update clock using message timestamp
            @source - source file descriptor
			returns - Message received
        */
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

    void writeReply(Message *m, int newsockfd, string text) {
    	/*
            Send a message - quick send
            @m - Message to be sent
            @newsockfd - source file descriptor
            @text - string to send
        */
        this->send(m->destination, newsockfd, text, m->sourceID, m->readWrite, m->fileName);
    }

    void connectAndReply(Message *m, int newsockfd, string text) {
    	/*
            Connect to a process and senc the message
            @m - Message
            @newsockfd - source file descriptor
            @text - string to send
        */
    	ProcessInfo p = findInVector(allClients, m->sourceID);
    	int fd = this->connectTo(p.hostname, p.port);
        writeReply(m, fd, text);
        close(fd);
    }

    void setClock() {
    	/*
            Update clock
        */
        this->clock += D;
    }

    void setClock(int timestamp) {
    	/*
            Update clock w.r.t received timestamp
        */
        this->clock = max(this->clock + D, timestamp + D);
    }

    void resetClock() {
    	/*
            RESET clock (not used)
        */
        this->clock = 0;
    }

    ~Socket() {
    	/*
            Destructor
        */
        close(personalfd);
    }
};