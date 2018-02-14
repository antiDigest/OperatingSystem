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

class Server: protected Socket {
    /*
        Server:
        * Manages files and file paths
        * Takes client requests to read and write
        * Does not do anything for maintaining mutual exclusion
    */

private:
     string directory;   

public:
    Server(char* argv[]): Socket(argv) {
        directory = argv[3];
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

            std::thread connectedThread(&Server::processMessages, this, newsockfd);
            connectedThread.detach();
        }
    }

    void processMessages(int newsockfd) {
        /*
            Starts as a thread which receives a message and checks the message
            @newsockfd - fd socket stream from which message would be received
        */
        try{
            Message* message = this->receive(newsockfd);
            this->checkMessage(message, newsockfd);
        } catch (const char* e){
            Logger(e, this->clock);
            close(newsockfd);
        }
    }

    void checkMessage(Message *m, int newsockfd) {
        /*
            Checks the message for different types of incoming messages
            1. hi - just a hello from some client/server (not in use)
            2. something else (means a request to be granted)
            @m - Message just received
            @newsockfd - socket stream it was received from
        */
        if (m->message == "hi") {
            writeReply(m , newsockfd, "hello");
        } else {
            this->checkReadWrite(m, newsockfd);
        }
        throw "BREAKING CONNECTION";
    }

    void checkReadWrite(Message *m, int newsockfd) {
        /*
            Checks the request type
            1 - Read: reads the last line of the file and sends it to the process
            2 - Write: Writes to the file and replies
            3 - Enquiry: replies with a list of files
            @m - Message just received
            @newsockfd - socket stream it was received from
        */
        switch (m->readWrite) {
        case 1: {
            string line = readFile(m->fileName);
            writeReply(m, newsockfd, line);
            break;
        }
        case 2: {
            string writeMessage = m->message + ", " + m->sourceID + ", " + globalTime();
            writeToFile(m->fileName, writeMessage);
            writeReply(m, newsockfd, m->message);
            break;
        }
        case 3: {
            vector<string> files = this->readDirectory(directory);
            writeReply(m, newsockfd, makeFileTuple(files));
            break;
        }
        default: {
            this->send(m->destination, m->source, "RECEIVED !", m->sourceID);
            break;
        }
        }
    }

    string readFile(string INPUT_FILE) {
        /*
            1 - Read: reads the last line of the file and sends it to the process
        */
        ifstream file(INPUT_FILE);
        string line;
        while (getline(file, line)) {}
        Logger("[RETURN READ FROM FILE]:" + line, this->clock);
        return line;
    }

    void writeToFile(string INPUT_FILE, string message) {
        /*
            2 - Write: Writes to the file and replies
        */
        ofstream file;
        file.open(directory + "/" + INPUT_FILE, ios::app);
        Logger("[WRITE TO FILE]:" + message, this->clock);
        file << message << endl;
        file.close();
    }

    vector<string> readDirectory(string name) {
        /*
            3 - Enquiry: replies with a list of files
        */
        vector<string> v;
        DIR* dirp = opendir(name.c_str());
        struct dirent *dp;
        while ((dp = readdir(dirp)) != NULL) {
            v.push_back(dp->d_name);
        }
        closedir(dirp);
        return v;
    }

};

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "usage %s ID port directory_path\n", argv[0]);
        exit(1);
    }

    Server *server = new Server(argv);

    std::thread listenerThread(&Server::listener, server);
    listenerThread.join();

    logger.close();
    return 0;
}