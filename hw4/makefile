all : main

main : 161044080_main.o
	gcc 161044080_main.o -pthread -lrt -o exe

161044080_main.o : 161044080_main.c
	gcc -c 161044080_main.c

run : 
	./exe

clean : 
	rm 161044080_main.o exe

