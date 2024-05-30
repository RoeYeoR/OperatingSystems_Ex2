CXX = g++
CXXFLAGS = -Wall -std=c++11

all: mync

mync: mync.o
	$(CXX) $(CXXFLAGS) -o mync mync.o

mync.o: mync.cpp
	$(CXX) $(CXXFLAGS) -c mync.cpp

clean:
	rm -f mync *.o
