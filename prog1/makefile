CXX = g++
CXXFLAGS = -Wall -std=c++11

all: ttt

ttt: TicTacToe.o
	$(CXX) $(CXXFLAGS) -o ttt TicTacToe.o

TicTacToe.o: TicTacToe.cpp TicTacToe.hpp
	$(CXX) $(CXXFLAGS) -c TicTacToe.cpp

clean:
	rm -f *.o ttt 
