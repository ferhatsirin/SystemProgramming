all : main

main : 161044080_main.o
	gcc 161044080_main.o -pthread -lm -o exe

161044080_main.o : 161044080_main.c
	gcc -c 161044080_main.c

run : 
	./exe data.dat

clean : 
	rm 161044080_main.o exe

