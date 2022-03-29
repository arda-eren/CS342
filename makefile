project2: systemsim.o
<<<<<<< HEAD
	gcc -o project2 systemsim.o -lpthread
=======
	gcc -o project2 systemsim.c -pthread
>>>>>>> f606d2cdbd669f1d571c803da5ffaa00d3080104
systemsim.o:
	gcc -c systemsim.c
clean:
	rm -f project2 systemsim.o