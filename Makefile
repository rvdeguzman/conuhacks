CXX = g++
CXXFLAGS = -Wall -std=c++11 -I$(HOME)/SDL/include -I/usr/local/include
LDFLAGS = -L$(HOME)/SDL/lib -L/usr/local/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lenet \
          -Wl,-rpath,$(HOME)/SDL/lib -Wl,-rpath,/usr/local/lib

all: server client

server: server.cpp common.h
	$(CXX) $(CXXFLAGS) server.cpp $(LDFLAGS) -o server

client: client.cpp SpriteSheet.cpp Menu.cpp Lobby.cpp common.h GameState.h Menu.h SpriteSheet.h Lobby.h
	$(CXX) $(CXXFLAGS) client.cpp SpriteSheet.cpp Menu.cpp Lobby.cpp $(LDFLAGS) -o client

clean:
	rm -f server client
