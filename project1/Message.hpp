// Message.hpp
/*
    @author: antriksh
    Version 0: 2/5/2018
*/

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <netdb.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <queue>
#include <chrono>
#include <ctime>
#include <time.h>
#include <dirent.h>

#define D 1

using namespace std;

class ProcessInfo {
public:
	int fd;
	string processID;
	string hostname;
	int port;
	string system;
};

class Message {
	/*
		Message: message being passed between two socket recipients.
	*/

public:
	string message;
	int readWrite;
	bool request;
	int source;
	string sourceID;
	int destination;
	int timestamp;
	string fileName;

	Message() {}

	Message(bool r, int rw, string m, int s, string sid, int d, int t) : message(m), request(r), readWrite(rw),
		source(s), sourceID(sid), destination(d), timestamp(t) {
		this->fileName = "NULL";
	}

	Message(bool r, int rw, string m, int s, string sid, int d, int t, string f) : message(m), request(r), readWrite(rw),
		source(s), sourceID(sid), destination(d), timestamp(t), fileName(f) {}

	bool operator<(const Message& rhs) {
		if (this->timestamp < rhs.timestamp) {
			return true;
		} else if (this->timestamp == rhs.timestamp) {
			if (this->source < rhs.source) {
				return true;
			}
		}
		return false;
	}
};

string messageString(Message *msg) {
	// cout << "Your Message: " << msg->message << endl;
	return to_string(msg->request) + ";" + to_string(msg->readWrite) + ";" + msg->message + ";" +
	       to_string(msg->source) + ";" + msg->sourceID + ";" + to_string(msg->destination) + 
	       ";" + to_string(msg->timestamp) + ";" + msg->fileName;
}

Message *getMessage(char* msg) {
	string message = msg;
	// vector<string> tokens = split(message, ";");
	istringstream line_stream(message);
	string token;
	vector<string> tokens;
	while (getline(line_stream, token, ';')) {
		tokens.push_back(token);
	}

	bool b;
	int rw, s, t, d;
	istringstream(tokens[0]) >> boolalpha >> b;
	message = tokens[2];
	rw = stoi(tokens[1]);
	s = stoi(tokens[3]);
	string sid = tokens[4];
	d = stoi(tokens[5]);
	t = stoi(tokens[6]);
	string fileName = tokens[7];

	Message *m = new Message(b, rw, message, s, sid, d, t, fileName);
	return m;
}

vector<ProcessInfo> readClients(vector<ProcessInfo> clients, string fileName) {
	ifstream clientFile(fileName);
	string line;
	while (getline(clientFile, line)) {
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

string randomFileSelect(vector<string> files) {
	int randomIndex = rand() % files.size();
	return files[randomIndex];
}

ProcessInfo findInVector(vector<ProcessInfo> clients, string name){
	for(ProcessInfo client: clients){
		if(client.processID == name)
			return client;
	}
	throw "Not found";
}


