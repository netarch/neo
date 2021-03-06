Plankton-neo
============

[![Travis](https://img.shields.io/travis/com/netarch/neo.svg)](https://travis-ci.com/netarch/neo)
[![GitHub workflow](https://github.com/netarch/neo/workflows/test/badge.svg)](https://github.com/netarch/neo/actions)
[![Codecov](https://img.shields.io/codecov/c/github/netarch/neo.svg)](https://codecov.io/gh/netarch/neo)
[![GitHub repo size](https://img.shields.io/github/repo-size/netarch/neo.svg)](https://github.com/netarch/neo)

Plankton-neo is a network testing tool based on the Plankton verification
framework. It combines explicit-state model checking and emulation techniques to
achieve high-coverage testing for softwarized networks with middlebox
components.

#### Table of Contents

- [Environment setup](#environment-setup)
    - [CMake](#cmake)
    - [SPIN](#spin)
- [Build and install Plankton-neo](#build-and-install-plankton-neo)
- [Usage](#usage)


## Environment setup

The following dependencies are needed for Plankton-neo.

- make
- cmake (>= 3.12)
- spin
- libnet
- modern C and C++ compilers (GCC or Clang)

`depends/setup.sh` can be used to set up the development environment, but
additional packages may be installed and it may overwrite existing packages in
the system. Some platforms may not yet be supported. Pull requests and issues
are appreciated.

### CMake

Please check that the cmake version is at least 3.12 with `cmake --version`. If
`depends/setup.sh` was used, the version should be correct. Otherwise, you can
install a newer version from the official pre-built releases. For example,

```sh
$ curl -LO "https://github.com/Kitware/CMake/releases/download/v3.19.6/cmake-3.19.6-Linux-x86_64.tar.gz"
$ tar xf cmake-3.19.6-Linux-x86_64.tar.gz
$ sudo cp -r cmake-3.19.6-Linux-x86_64/* /usr/
```

### SPIN

If your distribution doesn't have SPIN as a package, you may install it from the
[official GitHub repository](https://github.com/nimble-code/Spin).

## Build and install Plankton-neo

```sh
$ cd neo
$ git submodule update --init --recursive
$ cmake -B build -S . -DCMAKE_BUILD_TYPE=Release    # Release or Debug
$ cmake --build build -j$(nproc)
$ sudo cmake --install build
```

## Usage

```
Usage: neo [OPTIONS] -i <file> -o <dir>
Options:
    -h, --help           print this help message
    -a, --all            verify all ECs after violation
    -f, --force          remove pre-existent output directory
    -l, --latency        measure packet injection latencies
    -j, --jobs <N>       number of parallel tasks
    -e, --emulations <N> maximum number of emulation instances
    -i, --input <file>   network configuration file
    -o, --output <dir>   output directory
```

Examples:

```sh
$ neo -fj8 -i examples/00-reverse-path-filtering/network.toml -o output
```
