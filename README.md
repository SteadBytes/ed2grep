# ed2grep

http://www.cs.princeton.edu/courses/archive/spr08/cos333/ed_to_grep.html

## Requirements

- Use `ed.c` regular expression processing to build a version of `grep`
- `grep` should read input from `stdin` or from _one or more_ named files:
  - `grep regexpr [files...]`
- If > 2 files in argument list:
  - Each matched line should be prefixed by the filename and a colon (:)
- Else
  - Print only the matched lines
- `grep` options not required to be implemented
- Must return proper status values, as described in manual page _grep(1)_:
  ```
  0: One or more matches were found.
  1: No matches were found
  2. Syntax errors or innaccessible files (even if matches were found).
  ```
- **No** `goto` statements
- Remove uneeded variables, functions etc from `ed.c` source
- Use ANSI C
  - Proper function prototypes, header files etc
- Single file `grep.c`
- Can't use `system` or similar functions

## `grep` behaviour

```
$ grep root /etc/passwd
root:x:0:0:root:/root:/bin/bash

$ grep '^root:[[:alpha:]]' /etc/passwd
root:x:0:0:root:/root:/bin/bash

$ echo "hello" | grep hello
hello
```

## Approach

1.  Reduce `ed` to have only append, regex, print and quit functionality
2.  Ensure only minimal code is present to achieve step `1`
3.  Read stdin and apply regex without editor

  - Parse regex as single command line argument
  - Apply regex to stdin
  - Execute regex against stdin instead of editor buffer

  ```
  $ echo "some text" | grep text
  some text
  ```

5.  Read from file
  ```
  # file1.txt
  Hello, world!

  $ grep Hello file1.txt
  Hello, world!
  ```
6.  Read from multiple files
  ```
    # file1.txt
  Hello, world!
  
  # file2.txt
  Goodbye, world!

  $ grep Hello file1.txt file2.txt
  Hello, world!

  $ grep Goodbye file1.txt file2.txt
  Goodbye, world!

  $ grep [:punct:] file1.txt file2.txt
  Hello, world!
  Goodbye, world!
  ```
7. Add filenames to matches for multiple files
  ```
    # file1.txt
  Hello, world!

  # file2.txt
  Goodbye, world!

  $ grep [:punct:] file1.txt file2.txt
  file1.txt: Hello, world!
  file2.txt: Goodbye, world!
  ```
8. Ensure proper exit status codes
  ```
  $ echo "Hello, world!" | grep hello
  Hello, world!
  $ echo $?
  0

  $ echo "Hello, world!" | grep foo
  $ echo $?
  1

  $ echo "Hello, world!" | grep [:unkown-character-class:]
  $ echo $?
  1
  ```