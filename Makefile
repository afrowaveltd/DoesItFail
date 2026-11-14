# DoesItFail -- minimal reusable C17 test library.
CC ?= cc
AR ?= ar
CFLAGS ?= -std=c17 -O0 -g -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude
BUILD_DIR ?= build
LIBRARY = $(BUILD_DIR)/libdoesitfail.a
OBJS = $(BUILD_DIR)/test.o $(BUILD_DIR)/reporter_console.o

.PHONY: all help test demo demo-fail ncurses-test clean
.DEFAULT_GOAL := help

help:
	@printf 'DoesItFail (C17 seed)\n'
	@printf '  make all          - build static library\n'
	@printf '  make test         - run bootstrap tests (normal console, no ncurses)\n'
	@printf '  make demo         - run successful console demo\n'
	@printf '  make demo-fail    - intentionally failing demo (returns nonzero)\n'
	@printf '  make ncurses-test - reserved; interactive reporter not implemented yet\n'
	@printf '  make clean        - remove generated files\n'

all: $(LIBRARY)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/test.o: src/test.c include/does_it_fail/test.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/reporter_console.o: src/reporter_console.c include/does_it_fail/reporter_console.h include/does_it_fail/test.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIBRARY): $(OBJS)
	$(AR) rcs $@ $^

$(BUILD_DIR)/test_core: tests/test_core.c $(LIBRARY)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIBRARY) -o $@

test: $(BUILD_DIR)/test_core
	./$(BUILD_DIR)/test_core

$(BUILD_DIR)/console_demo: examples/console_demo.c $(LIBRARY)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIBRARY) -o $@

demo: $(BUILD_DIR)/console_demo
	./$(BUILD_DIR)/console_demo

demo-fail: $(BUILD_DIR)/console_demo
	./$(BUILD_DIR)/console_demo --fail

ncurses-test:
	@printf 'DoesItFail ncurses reporter is planned, not implemented. See Docs/ROADMAP.md.\n' >&2
	@exit 2

clean:
	rm -rf $(BUILD_DIR)
