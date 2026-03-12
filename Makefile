CC ?= gcc
CFLAGS ?= -g
LDFLAGS ?= -g -lm

BUILD ?= build

SRC=src/containers/bitset.c src/containers/hashmap.c src/containers/idlist.c src/containers/list.c src/containers/rectpack.c src/core.c src/file.c src/rand.c src/strconv.c src/stream.c src/string.c
OBJ=$(patsubst src/%.c,$(BUILD)/%.o,$(SRC))

all: $(BUILD)/libstd.a
$(BUILD)/libstd.a: $(OBJ)
	ar rcs $@ $^

$(BUILD)/%.o: src/%.c | $(BUILD)/containers
	$(CC) $(CFLAGS) -o $@ -c $^

$(BUILD):
	mkdir -p $@
$(BUILD)/containers:
	mkdir -p $@

clean:
	rm -rf $(BUILD)