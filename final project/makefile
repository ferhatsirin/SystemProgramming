all : main

main : server.o client.o
	gcc server.o -pthread -lm -o server
	gcc client.o -o client

server.o : server.c
	gcc -c server.c
	
client.o : client.c
	gcc -c client.c
	
runServer : 
	./server 5555 data.dat log.dat

runClient :
	./command

clean : 
	rm server.o client.o server client

