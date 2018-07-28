SHELL := /bin/bash

CC=gcc
CFLAGS=-ansi -fno-builtin
LDFLAGS=

SOURCES=grep.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=grep

all-tests = $(shell find tests/ -type f ! -name "*.*")

.PHONY: test

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE)

$(OBJECTS): $(SOURCES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(EXECUTABLE) $(OBJECTS)