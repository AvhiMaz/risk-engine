.PHONY: all build clean link

PROGRAM   := perps
SRC_DIR   := src
BUILD_DIR := build
INCLUDE   := ../thirdparty/caravel

SOLANA_VER := $(shell ls -d $(HOME)/.cache/solana/v*/platform-tools 2>/dev/null | sort -t/ -k7 -V | tail -1 | xargs dirname)
CLANG      ?= $(SOLANA_VER)/platform-tools/llvm/bin/clang
LLD        ?= $(SOLANA_VER)/platform-tools/llvm/bin/ld.lld

CFLAGS  := --target=sbf -fPIC -Oz -fno-builtin -fdata-sections -I$(INCLUDE)
LDFLAGS := -z notext -shared --Bdynamic --gc-sections $(INCLUDE)/bpf.ld --entry entrypoint

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
SO   := $(BUILD_DIR)/$(PROGRAM).so

all: $(SO)

$(SO): $(OBJS)
	$(LLD) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CLANG) $(CFLAGS) -c $< -o $@

build: all

link:
	bear -- make all

clean:
	rm -rf $(BUILD_DIR)
