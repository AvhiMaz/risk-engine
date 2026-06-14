.PHONY: engine program tools clean format

SOLANA_VER := $(shell ls -d $(HOME)/.cache/solana/v*/platform-tools 2>/dev/null | sort -t/ -k7 -V | tail -1 | xargs dirname)
CLANG      ?= $(SOLANA_VER)/platform-tools/llvm/bin/clang
LLD        ?= $(SOLANA_VER)/platform-tools/llvm/bin/ld.lld

PROGRAM   := perps
PROG_SRC  := $(shell find program/src -name '*.c')
PROG_HDRS := $(shell find program/src -name '*.h') $(shell find thirdparty/caravel -name '*.h')
PROG_OBJS := $(patsubst program/src/%.c, program/build/%.o, $(PROG_SRC))
PROG_SO   := program/build/$(PROGRAM).so
PROG_INC  := thirdparty/caravel

CFLAGS  := --target=sbf -fPIC -Oz -fno-builtin -fdata-sections -I$(PROG_INC)
LDFLAGS := -z notext -shared --Bdynamic --gc-sections $(PROG_INC)/bpf.ld --entry entrypoint

engine:
	gcc thirdparty/cjson/*.c thirdparty/libbase58/base58.c engine/src/*.c -o engine/main \
		-lpthread \
		-I thirdparty/cjson \
		-I thirdparty/libbase58 \
		-L/opt/homebrew/opt/libwebsockets/lib \
		-I/opt/homebrew/opt/libwebsockets/include \
		-lwebsockets \
		-L/opt/homebrew/opt/openssl@3/lib \
		-I/opt/homebrew/opt/openssl@3/include \
		-lssl -lcrypto \
		-lcurl

tools:
	gcc thirdparty/cjson/*.c thirdparty/libbase58/base58.c \
		engine/src/engine.c engine/src/keypair.c engine/src/rpc.c \
		tools/open_position.c -o tools/open_position \
		-lpthread \
		-I thirdparty/cjson \
		-I thirdparty/libbase58 \
		-I engine/src \
		-L/opt/homebrew/opt/openssl@3/lib \
		-I/opt/homebrew/opt/openssl@3/include \
		-lssl -lcrypto \
		-lcurl

program: $(PROG_SO)

$(PROG_SO): $(PROG_OBJS)
	$(LLD) $(LDFLAGS) -o $@ $^

program/build/%.o: program/src/%.c $(PROG_HDRS)
	@mkdir -p $(dir $@)
	$(CLANG) $(CFLAGS) -c $< -o $@

format:
	clang-format -i engine/src/*.c engine/src/*.h program/src/*.c program/src/*.h

clean:
	rm -f engine/main
	rm -rf program/build
