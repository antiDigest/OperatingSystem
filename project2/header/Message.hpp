// Message.hpp
/*
    @author: antriksh
    Version 0: 2/5/2018
    Version 1: 2/14/2018
                * Documentation improved
                * Added some more fields
                * Moved to a folder containing all headers
                * moved all include calls to a single header
                * left only the Message class related methods in this file
*/

#include "includes.hpp"

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
           msg->fileName;
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

    Message *m = new Message(rw, type, message, s, sid, d, t, fileName);
    return m;
}
