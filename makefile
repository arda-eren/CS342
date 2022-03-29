project2: systemsim.o
	gcc -pthread -o project2 systemsim.o
systemsim.o:
	gcc -c systemsim.c
clean:
	rm -f project2 systemsim.o