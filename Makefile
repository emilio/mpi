TARGET :=	bin/main

CC := mpicc

# Either "sync" or "async"
MODE ?= sync

# Job size, 5000 by default
JOB_SIZE ?= 5000

# crypt("10", "aa"); -> not found
# TEST_PASSWORDS := aal9/sIHZQyhA
TEST_PASSWORDS := aaTrgM6tVLhas aagWNRh7V9kN6 aahpg4OwfHMXY aaTwdzPnfU7XE aaxF36GTHquV

# TODO: Add a #define or command line argument for writing to csv, so it's
# easier to get stats and make that kind of fancy graphics that teachers like.
#
# A command line argument could be useful to avoid recompiling when changing it
# (or the job size).
#
# On the other hand, since we use argv for the passwords, and we should
# recompile to test with different round modes anyways... It might be easier to
# just throw it into a #define directive.
ifneq ($(MODE), sync)
	MODE_FLAGS := -DASYNC_ROUND
endif

CFLAGS := $(MODE_FLAGS) -DJOB_SIZE=$(JOB_SIZE) -Wall -pedantic -g -std=c99 -pedantic
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
