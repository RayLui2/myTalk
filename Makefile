CFLAGS = -Wall -g -O1

CC = gcc

mytalk: mytalk.o server.o
	$(CC) $(CFLAGS) -L ~pn-cs357/Given/Talk/lib64 -o mytalk mytalk.o server.o client.o -ltalk -lncurses

mytalk.o: mytalk.c server.c
	$(CC) $(CFLAGS) -I ~pn-cs357/Given/Talk/include -c mytalk.c server.c client.c

clean:
	rm *.o mytalk server
