#!/bin/sh

# Use this function to prepend to PATH an absolute name for each
# specified, possibly-$initial_cwd_-relative, directory.
path_prepend_ ()
{
  while test $# != 0; do
    path_dir_=$1
    PATH="$abs_path_dir_:$PATH"
    shift
  done
  export PATH
}