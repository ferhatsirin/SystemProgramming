all : main

main : 161044080_main.o ls.o cat.o wc.o prt.o
	gcc 161044080_main.o -lm -o shell
	gcc ls.o -o ls
	gcc cat.o -o cat
	gcc wc.o -o wc
	gcc prt.o -o prt

161044080_main.o : 161044080_main.c
	gcc -c 161044080_main.c
	
ls.o : ls.c
	gcc -c ls.c

cat.o : cat.c
	gcc -c cat.c
	
wc.o : wc.c
	gcc -c wc.c
	
prt.o : prt.c
	gcc -c prt.c

run : 
	./shell

clean : 
	rm *.o shell cat ls wc prt 

