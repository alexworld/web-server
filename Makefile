CC = g++
CFLAGS = -std=c++11 -Wpedantic -Werror -Wall -Wextra -Wshadow -Wconversion -O2
LDFLAGS = -lboost_system -lpthread

all : server

server : main.o
	$(CC) main.o $(LDFLAGS) -o server

main.o : main.cpp
	$(CC) $(CFLAGS) -c main.cpp

clean :
	rm -f server *.o
