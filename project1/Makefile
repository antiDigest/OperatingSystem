SERVER="server.cpp"
CLIENT="client.cpp"

ID="server1"
PORT="8000"
DIR="server1Directory"

all: server.out client.out

include: header/Message.hpp header/includes.hpp header/utils.hpp header/ProcessInfo.hpp

server.out: server.cpp client.cpp include
	g++ -std=c++11 -pthread server.cpp -o server.out

client.out: client.cpp server.cpp include
	g++ -std=c++11 -pthread client.cpp -o client.out

server: server.out
	./server.out $(ID) $(PORT) $(DIR)

client: client.out
	./client.out $(ID) $(PORT)