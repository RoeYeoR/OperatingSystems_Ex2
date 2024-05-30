CXX =g++
CXXFLAGS = -Wall -std=c++11

all: mync

mync:mync1.o
	$(CXX) $(CXXFLAGS) -o mync mync1.o

mync.o:mync1.cpp
	$(CXX) $(CXXFLAGS) -c mync1.cpp 


clean:
	rm -f *.o mync	


