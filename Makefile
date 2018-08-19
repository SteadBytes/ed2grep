SHELL := /bin/bash

CC = gcc
CFLAGS = -ansi -fno-builtin

SOURCES = grep.c
OBJECTS = $(SOURCES:.c=.o)
EXE = grep

# Debug build settings
DEBUGDIR = debug
DEBUGEXE = $(DEBUGDIR)/$(EXE)
DEBUGOBJECTS = $(addprefix $(DEBUGDIR)/, $(OBJECTS))
DEBUGCFLAGS = -O0 -ggdb3 -DDEBUG

# Release build settings
RELEASEDIR = release
RELEASEEXE = $(RELEASEDIR)/$(EXE)
RELEASEOBJECTS = $(addprefix $(RELEASEDIR)/, $(OBJECTS))
RELEASECFLAGS = -O3 -DNDEBUG

# Test settings
TESTDIR = tests
TESTS = $(shell find $(TESTDIR)/ -type f ! -name "*.*")

.PHONY: all clean debug prep release remake test

# Default build
all: prep release

# Release build rules
release: $(RELEASEEXE)

$(RELEASEEXE): $(RELEASEOBJECTS)
		$(CC) $(CFLAGS) $(RELEASECFLAGS) -o $(RELEASEEXE) $^

$(RELEASEDIR)/%.o: %.c
		$(CC) -c $(CFLAGS) $(RELEASECFLAGS) -o $@ $<

# Debug build rules
debug: $(DEBUGEXE)

$(DEBUGEXE): $(DEBUGOBJECTS)
	$(CC) $(CFLAGS) $(DEBUGCFLAGS) -o $(DEBUGEXE) $^

$(DEBUGDIR)/%.o: %.c
		$(CC) -c $(CFLAGS) $(DEBUGCFLAGS) -o $@ $<

# Other rules
prep:
		@mkdir -p $(DEBUGDIR) $(RELEASEDIR)
	
remake: clean all

clean:
		rm -f $(RELEASEEXE) $(RELEASEOBJECTS) $(DEBUGEXE) $(DEBUGOBJECTS)

check:
	fail=0; \
	for test in $(TESTS); do \
		export srcdir="$(TESTDIR)"; \
		export exedir="$(RELEASEDIR)"; \
		echo Running $$test...; \
		(. $$test) | sed -e 's/^/	/'; \
		if [ $${PIPESTATUS[0]} -ne 0 ]; then \
			echo $$test failed!; fail=1; \
		fi; \
	done; \
	exit $$fail