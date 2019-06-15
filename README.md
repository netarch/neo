Plankton-neo
============

[![Travis](https://img.shields.io/travis/com/netarch/neo.svg)](https://travis-ci.com/netarch/neo)
[![Codecov](https://img.shields.io/codecov/c/github/netarch/neo.svg)](https://codecov.io/gh/netarch/neo)
[![GitHub repo size](https://img.shields.io/github/repo-size/netarch/neo.svg)](https://github.com/netarch/neo)
[![License](https://img.shields.io/github/license/netarch/neo.svg)](https://github.com/netarch/neo/blob/master/LICENSE)

Plankton-neo is a network testing tool based on the Plankton verification
framework. It combines explicit-state model checking and emulation techniques to
achieve high-coverage testing for softwarized networks with middlebox
components.

#### Table of Contents

- [User's Guide](#users-guide)
    - [Dependencies](#dependencies)
    - [Install SPIN](#install-spin)
    - [Install Plankton-neo](#install-plankton-neo)
    - [Usage](#usage)
- [Developer's Guide](#developers-guide)


## User's Guide

### Dependencies

The following dependencies are needed for building Plankton-neo.

- autoconf
- make
- spin
- modern C and C++ compilers

It doesn't need anything in particular to run Plankton-neo, except for the C and
C++ standard libraries.

### Install SPIN

#### Arch Linux

SPIN is available from [AUR](https://aur.archlinux.org/packages/spin/).

```
$ git clone https://aur.archlinux.org/spin.git
$ cd spin
$ makepkg -srci
```

#### Ubuntu bionic and above

Since Ubuntu bionic (18.04), SPIN can be installed as a distribution package.

```
$ sudo apt install spin
```

#### Other

If your distribution doesn't have package for SPIN, you may install it from the
[tarball releases](http://spinroot.com/spin/Src/index.html) or the [official
GitHub repository](https://github.com/nimble-code/Spin). To make things easier,
you might want to run `setup.sh` for building and installing SPIN, though it
will also install other dependencies which may not be necessary for you.

### Install Plankton-neo

#### Build from source

```
$ cd neo
$ autoconf && ./configure && make -j
$ sudo make install
```

### Usage

```
Usage: neo [OPTIONS] -i <file> -o <dir>
Options:
    -h, --help             print this help message
    -v, --verbose          print more debugging information
    -f, --force            remove the output directory
    -j, --jobs <nprocs>    number of parallel tasks

    -i, --input <file>     network configuration file
    -o, --output <dir>     output directory
```

Examples:

```
$ cd neo
$ neo -vfj8 -i examples/00-reverse-path-filtering/network.toml -o output
```

## Developer's Guide

`setup.sh` can be used to set up the development environment. Pull requests and
issues are appreciated.

