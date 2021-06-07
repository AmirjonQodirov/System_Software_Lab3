
all: 
	$ gcc -c main.c ./utils/*.c ./utils/*.h ./client/*.h ./client/*.c ./server/*.h ./server/*.c
	$ mv ./*.o builds/
	$ gcc -o main ./builds/*.o -lcurses -lpthread
	$ chmod a+x main

clean: 
	$(RM) main ./builds/*.o
