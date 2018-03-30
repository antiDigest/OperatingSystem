// MetaInfo.hpp
/*
    @author: antriksh
    Version 0: 3/15/2018
*/

#include "includes.h"

// Store Meta-Data for easy access
class MetaInfo {
   public:
    string name;
    string chunkName;
    string server;

    MetaInfo(string name, string chunkName, string server)
        : name(name), chunkName(chunkName), server(server) {}

    string getChunkFile() { return name + "_" + chunkName; }
};

string getChunkFile(string name, string chunkName) {
    return name + "_" + chunkName;
}

// Converts the message from a string to MetaInfo
MetaInfo* stringToInfo(string line) {
    stringstream ss(line);
    string item;
    getline(ss, item, ':');
    string name = item;
    getline(ss, item, ':');
    string chunkName = item;
    getline(ss, item, ':');
    string server = item;

    MetaInfo* meta = new MetaInfo(name, chunkName, server);

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
    return m->name + ":" + m->chunkName + ":" + m->server;
}
