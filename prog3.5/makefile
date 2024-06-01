CXX = g++
CXXFLAGS = -Wall -std=c++11
COVFLAGS = -fprofile-arcs -ftest-coverage

TARGET = mync

.PHONY: all clean coverage

all: $(TARGET)

$(TARGET): mync.o
	$(CXX) $(CXXFLAGS) $(COVFLAGS) -o $@ $^

mync.o: mync.cpp 
	$(CXX) $(CXXFLAGS) $(COVFLAGS) -c $< -o $@

coverage: $(TARGET)
	./$(TARGET)
	gcov mync.cpp

clean:
	rm -f *.o $(TARGET) *.gcno *.gcda *.gcov



