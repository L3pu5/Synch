SRC_FILES=$(wildcard ./src/*.c)
OUT_FILE=./bin/synch

all: clean
	gcc $(SRC_FILES) -o $(OUT_FILE)

clean:
	rm ./bin/synch