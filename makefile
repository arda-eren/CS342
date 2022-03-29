project2: systemsim.o
	gcc -o project2 systemsim.o -lpthread
systemsim.o:
	gcc -c systemsim.c
clean:
	rm -f project2 systemsim.o