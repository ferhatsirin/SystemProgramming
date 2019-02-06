all : main

main : 161044080_main.o
	gcc 161044080_main.o -lm -o exe

161044080_main.o : 161044080_main.c
	gcc -std=c11 -Wall -c 161044080_main.c

run : 
	./exe test.tif

clean : 
	rm *.o exe

