# README.md


## Server file:

	"server.cpp"

### Server Directories:

	"server1Directory"
	"server2Directory"
	"server3Directory"

### TO RUN:

	"make server ID=<server ID> PORT=<port number> DIR=<server directory>"

## Client file:

	"client.cpp"

### TO RUN:

	"make server ID=<client ID> PORT=<port number>"

## Log files:

in folder:
	"logs/" - make this folder if not already present

## Add client information in file:

	"csvs/clients.csv"

## Add server information in file:

	"csvs/servers.csv"

## Format of csv:

	<process(client/server) id>,<ip address>,<port number>,<host machine: e.g. dc01, dc45 ..>


# INFO:

*	Make new directories for each server.
*	make a "logs/" folder in the directory having the Makefile/server.cpp/client.cpp