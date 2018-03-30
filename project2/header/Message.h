// Message.h
/*
    @author: antriksh
    Version 0: 3/14/2018
*/

#include "includes.h"

class Message {
    /*
            Message: message being passed between two socket recipients.
    */

   public:
    string type;
    string message;
    int readWrite;
    int source;
    string sourceID;
    int destination;
    int timestamp;
    string fileName;
    int offset = 0;
    int byteCount = 0;

    Message() {}

    Message(int rw, string type, string m, int s, string sid, int d, int t)
        : message(m),
          type(type),
          readWrite(rw),
          source(s),
          sourceID(sid),
          destination(d),
          timestamp(t) {
        this->fileName = "NULL";
    }

    Message(int rw, string type, string m, int s, string sid, int d, int t,
            string f)
        : type(type),
          message(m),
          readWrite(rw),
          source(s),
          sourceID(sid),
          destination(d),
          timestamp(t),
          fileName(f) {}

    Message(int rw, string type, string m, int s, string sid, int d, int t,
            string f, int offset, int byteCount)
        : type(type),
          message(m),
          readWrite(rw),
          source(s),
          sourceID(sid),
          destination(d),
          timestamp(t),
          fileName(f),
          offset(offset),
          byteCount(byteCount) {}

    // CHECK
    bool operator<(const Message &rhs) const {
        /*
                for priority queue check of which request has a higher priority
        */
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
    /*
            Converts message to tuple
    */
    return to_string(msg->readWrite) + ";" + msg->type + ";" + msg->message +
           ";" + to_string(msg->source) + ";" + msg->sourceID + ";" +
           to_string(msg->destination) + ";" + to_string(msg->timestamp) + ";" +
           msg->fileName + ";" + to_string(msg->offset) + ";" +
           to_string(msg->byteCount);
}

Message *getMessage(char *msg) {
    /*
            Converts char* tuple to Message
    */
    string message = msg;
    istringstream line_stream(message);
    string token;
    vector<string> tokens;
    while (getline(line_stream, token, ';')) {
        tokens.push_back(token);
    }

    int rw, s, t, d;
    string type = tokens[1];
    message = tokens[2];
    rw = stoi(tokens[0]);
    s = stoi(tokens[3]);
    string sid = tokens[4];
    d = stoi(tokens[5]);
    t = stoi(tokens[6]);
    string fileName = tokens[7];
    int offset = stoi(tokens[8]);
    int byteCount = stoi(tokens[9]);

    Message *m = new Message(rw, type, message, s, sid, d, t, fileName, offset,
                             byteCount);
    return m;
}
