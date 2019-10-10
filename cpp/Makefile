
# Default compiler and compiler flags
CXX=g++
CC=gcc

# Default flags for all compilers
O_FLAGS=-O0 -g2 -Wall -Wextra -pedantic
# Debugging flags
#O_FLAGS=-Og -g2 -Wall -Werror -Wextra -pedantic
CXX_FLAGS=$(O_FLAGS) -std=c++11
CC_FLAGS=$(O_FLAGS) -std=c99

# Default generic instructions
default:	all
all:	hdf5.o numeric
clean:	
	rm -f *.o

hdf5.o: hdf5.cpp hdf5.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< -I/usr/include/hdf5/serial/

numeric:	numeric.cpp
	$(CXX) $(CXX_FLAGS) -o $@ $<

