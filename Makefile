git clone https://github.com/libsdl-org/SDL
cd SDL
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel

#CMake >= 3.15
sudo cmake --install . --config Release
