SRC_FILES=$(wildcard ./src/*.c) $(wildcard ./src/*/*.c)
OUT_FILE=./bin/synch
LINKS=-lpthread -lssl -lcrypto

all: clean
	nc -lvnp 6969 > out &
	gcc $(SRC_FILES) -o $(OUT_FILE) $(LINKS) 
	./bin/synch

valgrind: clean
	gcc $(SRC_FILES) -o $(OUT_FILE) $(LINKS) 
	valgrind ./bin/synch



clean:
	rm -f ./bin/synch