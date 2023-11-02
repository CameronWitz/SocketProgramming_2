SRCS := servermain.cpp serverA.cpp serrverB.cpp serverC.cpp
OBJS := $(SRCS:.cpp=.o)

# Compiler and compilation flags
CC := g++
CFLAGS := -g -Wall

all: servermain serverA serverB serverC

servermain: servermain.o
	$(CC) $(CFLAGS) -o $@ $^

serverA: serverA.o
	$(CC) $(CFLAGS) -o $@ $^

serverB: serverB.o
	$(CC) $(CFLAGS) -o $@ $^
	
serverC: serverC.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) servermain server $(OBJS)
