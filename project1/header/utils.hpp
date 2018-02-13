// utils.hpp
/*
    @author: antriksh
    Version 0: 2/12/2018
*/

#include "includes.hpp"

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


string randomFileSelect(vector<string> files) {
	int randomIndex = rand() % files.size();
	return files[randomIndex];
}

string makeFileTuple(vector<string> files) {
    string allfiles = "";
    for (string file: files) {
        allfiles += file + ":";
    }
    cout << allfiles << endl;
    return allfiles.substr(0, allfiles.length() - 1);
}