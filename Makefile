PHONY: build run format all

build:
	gcc src/main.c src/engine.c src/scanner.c -o main

run:
	./main

format:
	clang-format -i src/*.c src/*.h

link:
	bear -- make build

all:
	make format && make build && make run
