TARGET :=	bin/main

CC := mpicc

# Either "sync" or "async"
MODE ?= sync

# crypt("10", "aa");
# TEST_PASSWORDS := aal9/sIHZQyhA
TEST_PASSWORDS := aaTrgM6tVLhas aagWNRh7V9kN6 aahpg4OwfHMXY aaTwdzPnfU7XE aaxF36GTHquV

ifneq ($(MODE), sync)
	MODE_FLAGS := -DASYNC_ROUND
endif

CFLAGS := $(MODE_FLAGS) -Wall -pedantic -g -std=c99 -pedantic
CLINKFLAGS := -lcrypt

NP ?= 4

OBJECTS := $(patsubst src/%, obj/%, \
           $(patsubst %.c, %.o, $(wildcard src/*.c)) \
           $(patsubst %.cpp, %.o, $(wildcard src/*.cpp)))

.PHONY: all
all: $(TARGET)
	@echo > /dev/null

.PHONY: clean
clean:
	$(RM) -r obj bin

.PHONY: autoformat
autoformat:
	for i in `find . -name '*.c'`; do echo "$$i"; clang-format "$$i" > "$$i.formatted"; mv "$$i.formatted" "$$i"; done
	for i in `find . -name '*.h'`; do echo "$$i"; clang-format "$$i" > "$$i.formatted"; mv "$$i.formatted" "$$i"; done

.PHONY: release
release: CFLAGS := $(CFLAGS) -O3
release: clean all

.PHONY: run
run: $(TARGET)
	mpirun -np $(NP) $(TARGET) $(TEST_PASSWORDS)

obj/%.o: src/%.c src/%.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

bin/%: obj/%.o $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(CLINKFLAGS)

bin/tests/%: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CC) -Isrc $(CFLAGS) $^ -o $@ $(CLINKFLAGS)
