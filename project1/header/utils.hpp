// utils.hpp
/*
    @author: antriksh
    Version 0: 2/12/2018
    Version 1: 2/14/2018
            * First Successful run !
            * Documentation improved
            * added getFiles function
*/

#include "includes.hpp"

using namespace std;

string logfile;
ofstream logger;

string globalTime() {
    /*
        returns a string of timestamp
    */

    time_t now = time(0);   // get time now
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void Logger(string message, int clock) {
    /*
        Logs to a file and atdout
        @message - string to log
        @clock - timestamp pf the process
    */
    logger << "[" << globalTime() << "][CLOCK: " << clock << "]::" << message << endl;
    cout << "[" << globalTime() << "][CLOCK: " << clock << "]::" << message << endl;
    logger.flush();
    return;
}

void error(const char *msg, int clock) {
    /*
        Logs and shows error messages
    */
    Logger(msg, clock);
    perror(msg);
    exit(1);
}

string randomFileSelect(vector<string> files) {
    /*
        returns a random file in the vector of files
    */
    int randomIndex = rand() % files.size();
    return files[randomIndex];
}

string makeFileTuple(vector<string> files) {
    /*
        returns a tuple from the vector of files
    */
    string allfiles = "";
    for (string file: files) {
        if(file=="." || file=="..")
            continue;
        allfiles += file + ":";
    }
    // cout << allfiles << endl;
    return allfiles.substr(0, allfiles.length() - 1);
}

vector<string> getFiles(string files) {
    /*
        returns a vector from a string of tuple containing all files
    */
    vector<string> allfiles;
    istringstream line_stream(files);
    string file;
    while (getline(line_stream, file, ':')) {
        allfiles.push_back(file);
    }
    return allfiles;
}