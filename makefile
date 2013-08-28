CFLAGS = -g
CC = gcc


mpchat: mpchat.o lista.o
	$(CC) $(CFLAGS) mpchat.o lista.o -o mpchat
        
mpchat.o: mpchat.c lista.h
	$(CC) $(CFLAGS) -c mpchat.c -o mpchat.o
     
lista.o: lista.c lista.h
	$(CC) $(CFLAGS) -c lista.c -o lista.o

clean:
	rm -f *~ *.o
