#!/bin/sh


finish()
{
  rm -f test.txt
}


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

output_match_test_failure()
{
  input=$1
  pattern=$2
  expected=$3
  got=$4
  name=$5

  printf "Match Test \"%s\" failed\n\tinput: %s\n\tpattern: %s\n\texpected: %s\n\tgot: %s\n" "$name" "$input" "$pattern" "$expected" "$got" >&2
}

match_test()
{
  test_fail=0
  input=$(echo -e $1)
  pattern=$2
  expected=$(echo -e $3)
  name=$4

  out=$(echo -e "$input" | grep "$pattern")
  if test "$out" != "$expected"; then
    output_match_test_failure "$input" "$pattern" "$expected" "$out" "$name (stdin)"
    test_fail=1
  fi

  echo -e "$input" > test.txt 
  out=$(grep "$pattern" test.txt)
  if test "$out" != "$expected"; then
    output_match_test_failure "$input" "$pattern" "$expected" "$out" "${name} (file)"
    test_fail=1
  fi

  if [ $fail -eq 0 ] && [ $test_fail -eq 1 ]; then
    fail=1
  fi

  return $test_fail
}

output_status_test_failure()
{
  input=$1
  pattern=$2
  expected=$3
  got=$4
  name=$5

  printf "Status Test \"%s\" failed\n\tinput: %s\n\tpattern: %s\n\texpected status: %s\n\tgot: %s\n" "$name" "$input" "$pattern" "$expected" "$got" >&2
}

status_test()
{
  test_fail=0
  input=$1
  pattern=$2
  expected_status=$3
  name=$4

  out=$(echo -e "$input" | grep "$pattern")
  status=$?
  if test $status -ne $expected_status; then
    output_status_test_failure "$input" "$pattern" $expected_status $status "$name (stdin)"
    test_fail=1
  fi

  echo -e "$input" > test.txt 
  out=$(grep "$pattern" test.txt)
  status=$?
  if test $status -ne $expected_status; then
    output_status_test_failure "$input" "$pattern" $expected_status $status "${name} (file)"
    test_fail=1
  fi

  if [ $fail -eq 0 ] && [ $test_fail -eq 1 ]; then
    fail=1
  fi

  return $test_fail
}