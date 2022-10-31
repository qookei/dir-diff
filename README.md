# dir-diff

dir-diff is an utility that computes the difference between two directories and
displays it in a human-friendly format.

## Building

dir-diff is a regular [Meson](https://mesonbuild.com) project, with no extra
compile-time options.

To build it, you will need:
 - a C++20 compiler,
 - the [Botan](https://botan.randombit.net/index.html) library.

## Usage

When invoked with the wrong number of arguments, the tool will print the
following usage message:

```
usage: <path 1> <path 2>
```

The two paths should point to the directories whose contents are to be compared.

When a difference is detected, it will first print a legend, and the diff below.

## How it works

The tool first builds an in-memory representation of both file system trees
specified, including computing hashes of every file (for directories, the hashes
and names of each child are used to compute a hash). The hash algorithm used is
Blake2b.

Next, after both trees have been processed, the tool starts at the root, and:
1. builds a union of the sets of files in both directories,
2. checks which files mismatch (either by not existing in one, being different
   kinds of files, or by contents),
3. displays the difference, additionally repeating this process for subtrees.

## License

This project is licensed under the GPLv3 (or later) license.
