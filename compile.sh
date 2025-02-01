#!/bin/bash

g++ client.cpp server.cpp raycast.cpp -Wall -std=c++11 -I/home/frouque/SDL/include -I/usr/local/include \
-L/home/frouque/SDL/lib -L/usr/local/lib -lSDL2 -lSDL2_image -lenet \
-Wl,-rpath,/home/frouque/SDL/lib -Wl,-rpath,/usr/local/lib -o client && echo "Compilation finished."
