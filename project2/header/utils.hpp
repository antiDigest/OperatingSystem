// utils.hpp
/*
    @author: antriksh
    Version 0: 3/14/2018
*/

#include "ProcessInfo.hpp"
#include "includes.hpp"

using namespace std;

string logfile;
ofstream logger;

// returns a string of timestamp
string globalTime() {
    time_t now = time(0);  // get time now
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
// Logs to a file and stdout
// @message - string to log
void Logger(string message) {
    logger << "[" << globalTime() << "]::" << message << endl;
    cout << "[" << globalTime() << "]::" << message << endl;
    logger.flush();
    return;
}

// Logs and shows error messages
void error(const char *msg) {
    Logger(msg);
    perror(msg);
    exit(1);
}

// returns a random file in the vector of files
string randomFileSelect(vector<string> files) {
    int randomIndex = rand() % files.size();
    return files[randomIndex];
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

// returns a tuple from the vector of files
string makeFileTuple(vector<string> files) {
    string allfiles = "";
    for (string file : files) {
        if (file == "." || file == "..") continue;
        allfiles += file + ":";
    }
    // cout << allfiles << endl;
    return allfiles.substr(0, allfiles.length() - 1);
}

// returns a vector from a string of tuple containing all files
vector<string> getFiles(string files) {
    vector<string> allfiles;
    istringstream line_stream(files);
    string file;
    while (getline(line_stream, file, ':')) {
        allfiles.push_back(file);
    }
    return allfiles;
}