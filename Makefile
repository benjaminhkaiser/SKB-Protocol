CC=g++
CFLAGS=-I. -lcryptopp

all:
	$(CC) atm.cpp util.cpp -m32 -o atm $(CFLAGS)
	$(CC) bank.cpp account.cpp util.cpp -m32 -o bank -lpthread $(CFLAGS)
	$(CC) proxy.cpp -m32 -o proxy -lpthread $(CFLAGS)
