Plankton-neo
============

[![Travis](https://img.shields.io/travis/com/netarch/neo.svg)](https://travis-ci.com/netarch/neo)
[![Codecov](https://img.shields.io/codecov/c/github/netarch/neo.svg)](https://codecov.io/gh/netarch/neo)
[![GitHub repo size](https://img.shields.io/github/repo-size/netarch/neo.svg)](https://github.com/netarch/neo)
[![License](https://img.shields.io/github/license/netarch/neo.svg)](https://github.com/netarch/neo/blob/master/LICENSE)

Plankton-neo is a network testing tool based on the Plankton verification
framework, using both explicit-state model checking and emulation techniques to
achieve high-coverage testing for softwarized networks.


## Getting Started

The following dependencies are needed for building Plankton-neo.

- autoconf
- make
- spin

### Install SPIN

#### Ubuntu bionic and above

Since Ubuntu bionic (18.04), SPIN can be installed as a distribution package.

```
$ sudo apt install spin
```

#### Arch Linux

SPIN is available from [AUR](https://aur.archlinux.org/packages/spin/).

```
$ git clone https://aur.archlinux.org/spin.git
$ cd spin
$ makepkg -srci
```

#### Other

If your distribution doesn't have the package for SPIN, you may install it from
the [tarball releases](http://spinroot.com/spin/Src/index.html) or the [GitHub
repository](https://github.com/nimble-code/Spin).

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

