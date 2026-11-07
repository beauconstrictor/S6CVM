CC = clang
CFLAGS = -Wall -Wextra -Werror
DEPFLAGS = -MMD -MP
TARGET = build/s6cvm

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))

.PHONY: all
all: $(TARGET)

build:
	mkdir -p build

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) $(DEPFLAGS) -Isrc -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

.PHONY: clean
clean:
	rm -rf build/

.PHONY: run
run: $(TARGET)
	$(TARGET)

DEPS = $(OBJS:.o=.d)
-include $(DEPS)
