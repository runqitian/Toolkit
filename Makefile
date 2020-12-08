CC = g++
CC_FLAGS = -std=c++11

.PHONY: all
all: rqtrans

rqtrans: src/*
	mkdir -p bin
	$(CC) $(CC_FLAGS) -g -pthread src/rqtrans/*.cpp -o bin/$@

clean:
	rm -rf bin/*
