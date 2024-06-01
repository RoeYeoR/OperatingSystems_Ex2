CC = g++
CFLAGS = -Wall -Wextra -Werror -std=c++11 -O2
COVFLAGS = -fprofile-arcs -ftest-coverage
TARGET = ttt

.PHONY: all clean coverage

all: $(TARGET)

$(TARGET): ttt.o
	$(CC) $(CFLAGS) $(COVFLAGS) -o $@ $^

ttt.o: ttt.cpp ttt.hpp
	$(CC) $(CFLAGS) $(COVFLAGS) -c ttt.cpp	

coverage: $(TARGET)
	
	gcov ttt.cpp

clean:
	rm -f $(TARGET) *.gcno *.gcda *.gcov *.o