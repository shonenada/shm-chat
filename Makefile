all : client server

client : src/client.c src/structs.h
	mkdir -p target
	gcc src/client.c -o target/client

server : src/server.c src/structs.h src/parser.h
	mkdir -p target
	gcc src/server.c -o target/server
