CXX = g++
CXXFLAGS = -Wall -std=c++11

all: mync

mync: mync2.o
	$(CXX) $(CXXFLAGS) -o mync mync2.o

mync.o: mync2.cpp
	$(CXX) $(CXXFLAGS) -c mync2.cpp

clean:
	rm -f *.o mync		