CXX = g++
CXXFLAGS = -Wall -std=c++11

all: mync_3.5

mync_3.5: mync.o
	$(CXX) $(CXXFLAGS) -o mync_3.5 mync.o

mync.o: mync.cpp
	$(CXX) $(CXXFLAGS) -c mync.cpp

clean:
	rm -f mync_3.5 *.o
