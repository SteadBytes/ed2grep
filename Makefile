SHELL := /bin/bash

CC=gcc
CFLAGS=-ansi -fno-builtin
LDFLAGS=

SOURCES=grep.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=grep

all-tests = $(shell find tests/ -type f ! -name "*.*")


all: $(EXECUTABLE)

.PHONY: test

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE)

$(OBJECTS): $(SOURCES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(EXECUTABLE) $(OBJECTS)

test:
	fail=0; \
	for test in $(all-tests); do \
		export srcdir="tests/"; \
		echo Running $$test...; \
		(. $$test) | sed -e 's/^/	/'; \
		if [ $${PIPESTATUS[0]} -ne 0 ]; then \
			echo $$test failed!; fail=1; \
		fi; \
	done; \
	exit $$fail