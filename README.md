# dir-diff

dir-diff is an utility that computes the difference between two directories and
displays it in a human-friendly format.

## Building

dir-diff is a regular [Meson](https://mesonbuild.com) project, with no extra
compile-time options.

To build it, you will need:
 - a C++20 compiler.
 - [fmt](https://github.com/fmtlib/fmt) if your C++ standard library does not
   provide `std::format`/`std::print`.

## Usage

Basic usage is as follows:

```
$ dir-diff dir1 dir2
```

The two paths should point to the directories whose contents are to be compared.

For a list of options, see `dir-diff --help`.

When a difference is detected, the program will first print a legend, and the diff
below.

## How it works

The tool starts at the root of both directories, and first builds a union of the
sets of files in both directories.

Then checks which files mismatch. The way the mismatch is detected is as follows
(the steps are ordered in a way where if a match/mismatch is detected, further checks
are skipped):

1. the existence of the file in both directories is checked (mismatch if missing in one),
2. the file type is compared (mismatch if not equal),
3. the file contents are compared, for directories this means applying the same
   algorithm recursively, while for other file types:
     1. the file sizes are compared (regular files only, mismatch if not equal),
     2. the inode and device numbers they're on are compared (assumed same if numbers match),
     3. the link targets are compared (for symlinks),
     4. the file contents are compared (for regular files),
     5. the target device numbers are compared (for special files).

## License

This project is licensed under the GPLv3 (or later) license.
