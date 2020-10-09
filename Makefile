CC = g++
CC_FLAGS = -std=c++11

.PHONY: all
all: rqtrans

rqtrans: src/*
	mkdir -p bin
	$(CC) $(CC_FLAGS) -pthread src/rqtrans/* -o bin/$@

clean:
	rm -rf bin/*