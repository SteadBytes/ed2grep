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
