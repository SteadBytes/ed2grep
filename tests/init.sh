#!/bin/sh

# Use this function to prepend to PATH an absolute name for each
# specified, possibly-$initial_cwd_-relative, directory.
path_prepend_ ()
{
  while test $# != 0; do
    path_dir_=$1
    PATH="$path_dir_:$PATH"
    shift
  done
  export PATH
}
trap set_fail ERR

trap finish EXIT

# Use this function to prepend to PATH an absolute name for each
# specified, possibly-$initial_cwd_-relative, directory.
path_prepend_ ()
{
  while test $# != 0; do
    path_dir_=$1
    PATH="$path_dir_:$PATH"
    shift
  done
  export PATH
}

output_test_failure()
{
  input=$1
  pattern=$2
  expected=$3
  name=$4

  printf "Test \"%s\" failed\n\tinput: %s\n\tpattern: %s\n\texpected: %s\n" "$name" "$input" "$pattern" "$expected" >&2
}

match_test()
{
  input=$1
  pattern=$2
  expected=$3
  name=$4

  out=$(echo -e "$input" | grep "$pattern")
  if test "$out" != "$expected"; then
    output_test_failure "$input" "$pattern" "$expected" "$name (stdin)"
    return 1
  fi

  echo -e "$input" > test.txt 
  out=$(grep "$pattern" test.txt)
  if test "$out" != "$expected"; then
    output_test_failure "$input" "$pattern" "$expected" "${name} (file)"
    return 1
  fi

  return 0
}

status_test()
{
  input=$1
  pattern=$2
  status=$3
  name=$4

  out=$(echo -e "$input" | grep "$pattern")
  if test $? -ne $status; then
    output_test_failure "$input" "$pattern" $status "$name (stdin)"
    return 1
  fi

  echo -e "$input" > test.txt 
  out=$(grep "$pattern" test.txt)
  if test $? -ne $status; then
    output_test_failure "$input" "$pattern" $status "${name} (file)"
    return 1
  fi

  return 0
}