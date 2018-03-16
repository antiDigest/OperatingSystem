// ProcessInfo.hpp
/*
    @author: antriksh
    Version 0: 3/14/2018
*/

#include "includes.hpp"

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
    time_t aliveTime = time(NULL);
    bool alive = true;
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
        // ProcessInfo c = client.load();
        if (client.processID == name) return client;
    }
    throw "Not found";
}