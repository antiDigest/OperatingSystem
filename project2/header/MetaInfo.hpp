// MetaInfo.hpp
/*
    @author: antriksh
    Version 0: 3/15/2018
*/

#include "includes.hpp"

// Store Meta-Data for easy access
class MetaInfo {
   public:
    string name;
    string chunkName;
    string server;
    int size;
    int lines;

    MetaInfo(string name, string chunkName, string server, int size, int lines)
        : name(name),
          chunkName(chunkName),
          server(server),
          size(size),
          lines(lines) {}

    string getChunkFile() { return name + "_" + chunkName; }
};

// Converts the message from a string to MetaInfo
MetaInfo* stringToInfo(string line) {
    MetaInfo* meta;
    stringstream ss(line);
    string item;
    getline(ss, item, ':');
    meta->name = item;
    getline(ss, item, ':');
    meta->chunkName = item;
    getline(ss, item, ':');
    meta->server = item;
    getline(ss, item, ':');
    meta->size = stoi(item);
    getline(ss, item, ':');
    meta->lines = stoi(item);

    return meta;
}

// Read and store information of all processes in a given fileName
vector<MetaInfo*> getMetaInfo(string fileName) {
    ifstream metaFile(fileName);
    vector<MetaInfo*> metas;
    string line;
    while (getline(metaFile, line)) {
        MetaInfo* m = stringToInfo(line);
        metas.push_back(m);
    }
    return metas;
}

// Converts info to tuple
string infoToString(MetaInfo* m) {
    return m->name + ":" + m->chunkName + ":" + m->server + ":" +
           to_string(m->size) + ":" + to_string(m->lines);
}
