CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -Werror
LFLAGS = -Wall -Wextra -Wpedantic -Werror
BUILD_DIR ?= build

build:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c core.c -o $(BUILD_DIR)/core.o

clean:
	rm build/
	rm core.o