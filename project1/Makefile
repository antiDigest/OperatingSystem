SERVER="server.cpp"
CLIENT="client.cpp"

ID="server1"
PORT="8000"
DIR="server1Directory"

all: server.out client.out

server.out: server.cpp client.cpp Message.hpp
	g++ -std=c++11 -pthread server.cpp -o server.out

client.out: client.cpp server.cpp Message.hpp
	g++ -std=c++11 -pthread client.cpp -o client.out

server: server.out
	./server.out $(ID) $(PORT) $(DIR)

client: client.out
	./client.out $(ID) $(PORT)