// ProcessInfo.hpp
/*
    @author: antriksh
    Version 0: 2/12/2018
*/

#include "includes.hpp"

class ProcessInfo {
public:
	int fd;
	string processID;
	string hostname;
	int port;
	string system;
	bool repliedRead = false;
	bool repliedWrite = false;
};


vector<ProcessInfo> readClients(vector<ProcessInfo> clients, string fileName) {
	ifstream clientFile(fileName);
	string line;
	while (getline(clientFile, line)) {
		if (line.c_str()[0] == '#')
			continue;
		ProcessInfo client;
		stringstream ss(line);
		string item;
		getline(ss, item, ',');
		client.processID = item;
		getline(ss, item, ',');
		client.hostname = item;
		getline(ss, item, ',');
		client.port = stoi(item);
		getline(ss, item, ',');
		client.system = item;
		clients.push_back(client);
	}
	return clients;
}

ProcessInfo findInVector(vector<ProcessInfo> clients, string name) {
	for (ProcessInfo client: clients) {
		if (client.processID == name)
			return client;
	}
	throw "Not found";
}