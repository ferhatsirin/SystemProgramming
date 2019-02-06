all : main

main : 161044080_main.o
	gcc 161044080_main.o -lm -o exe

161044080_main.o : 161044080_main.c
	gcc -c 161044080_main.c

run : 
	./exe -n 5 -x file.dat -m 10

clean : 
	rm *.o exe

