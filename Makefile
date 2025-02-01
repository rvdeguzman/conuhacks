CXX = g++
CXXFLAGS = -Wall -std=c++11 -I$(HOME)/SDL/include -I/usr/local/include
LDFLAGS = -L$(HOME)/SDL/lib -L/usr/local/lib -lSDL2 -lSDL2_image -lenet \
          -Wl,-rpath,$(HOME)/SDL/lib -Wl,-rpath,/usr/local/lib

all: server client

server: server.cpp common.h
	$(CXX) $(CXXFLAGS) server.cpp $(LDFLAGS) -o server

client: client.cpp common.h
	$(CXX) $(CXXFLAGS) client.cpp $(LDFLAGS) -o client

clean:
	rm -f server client
