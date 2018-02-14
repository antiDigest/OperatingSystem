/*
    @author: antriksh
    Version 0: 2/5/2018
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
        while (1) {
            try{
                Message* message = this->receive(newsockfd);
                this->checkMessage(message, newsockfd);
            } catch (const char* e){
                Logger(e, this->clock);
                close(newsockfd);
                break;
            }
        }
        // close(newsockfd);
    }

    void checkMessage(Message *m, int newsockfd) {
        if (m->message == "hi") {
            writeReply(m , newsockfd, "hello");
        } else {
            this->checkReadWrite(m, newsockfd);
        }
        throw "BREAKING CONNECTION";
    }

    void checkReadWrite(Message *m, int newsockfd) {
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
        ifstream file(INPUT_FILE);
        string line;
        while (getline(file, line)) {}
        Logger("[RETURN READ FROM FILE]:" + line, this->clock);
        return line;
    }

    void writeToFile(string INPUT_FILE, string message) {
        ofstream file;
        file.open(directory + "/" + INPUT_FILE, ios::app);
        Logger("[WRITE TO FILE]:" + message, this->clock);
        file << message << endl;
        file.close();
    }

    vector<string> readDirectory(string name) {
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