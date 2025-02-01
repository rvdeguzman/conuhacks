
CXX = g++
CXXFLAGS = -Wall -std=c++11 -I$(HOME)/SDL/include -I/usr/local/include
LDFLAGS = -L$(HOME)/SDL/lib -L/usr/local/lib -lSDL2 -lenet -Wl,-rpath,$(HOME)/SDL/lib -Wl,-rpath,/usr/local/lib

all: server client

server: server.cpp common.h
	$(CXX) server.cpp $(CXXFLAGS) $(LDFLAGS) -o server

client: client.cpp common.h
	$(CXX) client.cpp $(CXXFLAGS) $(LDFLAGS) -o client

clean:
	rm -f server client
