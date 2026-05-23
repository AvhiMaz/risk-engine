.PHONY: build run format all

build:
	gcc ../thirdparty/cjson/*.c src/*.c -o main -lpthread -I ../thirdparty/cjson -L/opt/homebrew/opt/libwebsockets/lib -I/opt/homebrew/opt/libwebsockets/include -lwebsockets -L/opt/homebrew/opt/openssl@3/lib -I/opt/homebrew/opt/openssl@3/include -lssl -lcrypto

run:
	./main

format:
	clang-format -i src/*.c src/*.h

link:
	bear -- make build

all:
	make format && make build && make run
