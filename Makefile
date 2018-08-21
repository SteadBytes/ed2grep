SHELL := /bin/bash

CC = gcc
CFLAGS = -ansi -fno-builtin

SOURCES = grep.c
OBJECTS = $(SOURCES:.c=.o)
EXE = grep

BUILDSDIR = build

# Debug build settings
DEBUGDIR = $(BUILDSDIR)/debug
DEBUGEXE = $(DEBUGDIR)/$(EXE)
DEBUGOBJECTS = $(addprefix $(DEBUGDIR)/, $(OBJECTS))
DEBUGCFLAGS = -O0 -ggdb3 -DDEBUG

# Release build settings
RELEASEDIR = $(BUILDSDIR)/release
RELEASEEXE = $(RELEASEDIR)/$(EXE)
RELEASEOBJECTS = $(addprefix $(RELEASEDIR)/, $(OBJECTS))
RELEASECFLAGS = -O3 -DNDEBUG

# Test build settings
TESTDIR = $(BUILDSDIR)/test
TESTEXE = $(TESTDIR)/$(EXE)
TESTOBJECTS = $(addprefix $(TESTDIR)/, $(OBJECTS))
TESTCFLAGS = -fprofile-arcs -ftest-coverage

# Test suite settings
TESTSDIR = tests
TESTS = $(shell find $(TESTSDIR)/ -type f ! -name "*.*")

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

# Test build rules
test: $(TESTEXE)

$(TESTEXE): $(TESTOBJECTS)
	$(CC) $(CFLAGS) $(TESTCFLAGS) -o $(TESTEXE) $^

$(TESTDIR)/%.o: %.c
		$(CC) -c $(CFLAGS) $(TESTCFLAGS) -o $@ $<

# Other rules
prep:
		@mkdir -p $(DEBUGDIR) $(RELEASEDIR) $(TESTDIR)
	
remake: clean all

clean:
		rm -f $(RELEASEEXE) $(RELEASEOBJECTS) $(DEBUGEXE) $(DEBUGOBJECTS)

check:
	make test
	fail=0; \
	for test in $(TESTS); do \
		export srcdir="$(TESTSDIR)"; \
		export exedir="$(TESTDIR)"; \
		echo Running $$test...; \
		(. $$test) | sed -e 's/^/	/'; \
		if [ $${PIPESTATUS[0]} -ne 0 ]; then \
			echo $$test failed!; fail=1; \
		fi; \
	done; \
	if [ $$fail -eq 0 ]; then \
		gcov grep.c -o "$(TESTDIR)"; \
	fi; \
	exit $$fail