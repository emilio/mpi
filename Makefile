TARGET :=	bin/main
TESTS := bin/tests/main

CC := mpicc

# crypt("10", "aa");
# TEST_PASSWORDS := aal9/sIHZQyhA
TEST_PASSWORDS := aaTrgM6tVLhas aagWNRh7V9kN6 # aahpg4OwfHMXY aaTwdzPnfU7XE aaxF36GTHquV

COMMON_FLAGS := -Wall -pedantic -g

CFLAGS := $(COMMON_FLAGS) -std=c99 -pedantic
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

.PHONY: test
test: $(TESTS)
	mpirun -np $(NP) $<

.PHONY: run
run: $(TARGET)
	mpirun --mca btl tcp,self -np $(NP) $(TARGET) $(TEST_PASSWORDS)

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
