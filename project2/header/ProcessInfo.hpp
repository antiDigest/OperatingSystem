// ProcessInfo.hpp
/*
    @author: antriksh
    Version 0: 3/14/2018
*/

#include "includes.h"

// Store information of a process
class ProcessInfo {
   public:
    int fd;
    string processID;
    string hostname;
    int port;
    string system;
    bool repliedRead;
    bool repliedWrite;
    time_t aliveTime = time(0);
    bool alive = true;
    vector<string> files;

    void setAlive() {
        alive = true;
        aliveTime = time(0);
    }

    void checkAlive() {
        float aliveLast = (time(0) - getAliveTime());
        if (alive && aliveLast > 15.0) {
            Logger("[" + processID + " has missed 3 heartbeats]");
            resetAlive();
        }
    }

    void resetAlive() { alive = false; }

    bool getAlive() { return alive; }

    time_t getAliveTime() { return aliveTime; }

    void addFile(string name) { files.push_back(name); }

    void updateFiles(string f) {
        string filestring = makeFileTuple(files);
        if (filestring != f) {
            files = getFiles(f);
        }
    }
};

// Read and store information of all processes in a given fileName
vector<ProcessInfo> readClients(vector<ProcessInfo> clients, string fileName) {
    ifstream clientFile(fileName);
    string line;
    while (getline(clientFile, line)) {
        if (line.c_str()[0] == '#') continue;
        // ProcessInfo client;
        ProcessInfo c;
        stringstream ss(line);
        string item;
        getline(ss, item, ',');
        c.processID = item;
        getline(ss, item, ',');
        c.hostname = item;
        getline(ss, item, ',');
        c.port = stoi(item);
        getline(ss, item, ',');
        c.system = item;
        // client.store(c);
        clients.push_back(c);
    }
    return clients;
}

// Find an ID in a vector of clients/servers
ProcessInfo findInVector(vector<ProcessInfo> clients, string name) {
    for (ProcessInfo client : clients) {
        if (client.processID == name) return client;
    }
    throw "Not found";
}

// Find the index of ID in a vector of clients/servers
int findServerIndex(vector<ProcessInfo> clients, string name) {
    int index = 0;
    for (ProcessInfo client : clients) {
        if (client.processID == name) return index;
        index++;
    }
    return -1;
}

// Find if a server has a file in a vector of ProcessInfo (servers)
ProcessInfo findFileServer(vector<ProcessInfo> clients, string file) {
    for (ProcessInfo client : clients) {
        vector<string> files = client.files;
        for (string f : files) {
            if (f == file) return client;
        }
    }
    throw "File not found on any active servers";
}

// Check if all of the servers are dead
bool allDead(vector<ProcessInfo> set) {
    int deadCount = 0;
    for (ProcessInfo server : set) {
        if (!server.alive) {
            deadCount++;
        }
    }

    return deadCount == set.size();
}

// returns a random Process in the vector
ProcessInfo randomSelect(vector<ProcessInfo> set) {
    if (allDead(set)) throw "ALL SERVERS ARE DEAD";

    int randomIndex = rand() % set.size();
    if (set[randomIndex].alive)
        return set[randomIndex];
    else
        return randomSelect(set);
}