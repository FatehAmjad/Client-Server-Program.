CC=gcc
CFLAGS=-O3 -Wall 
LIBS=-pthread
SRC=files

all: Server Client
Server: server.o
	$(CC) $(CFLAGS) $(LIBS) -o Server server.o
Client: client.o text.o
	$(CC) $(CFLAGS) $(LIBS) -o Client client.o text.o
server.o: $(SRC)/server.c
	$(CC) $(CFLAGS) -c $(SRC)/server.c
text.o: $(SRC)/text.c
	$(CC) $(CFLAGS) -c $(SRC)/text.c
client.o: $(SRC)/client.c
	$(CC) $(CFLAGS) -c $(SRC)/client.c

.PHONY: clean
clean:
	rm -f *.o Server Client