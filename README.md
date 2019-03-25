# Felicia

[![Build Status](https://travis-ci.com/chokobole/felicia.svg?token=uWEvhLXsK9nuPxhDRPic&branch=master)](https://travis-ci.com/chokobole/felicia)

**Cross platform**, **Secure**, **Productive** robot framework.

## Prerequisites

We use [bazel](https://www.bazel.build/) as a build tool. So please download it! To sync with window build, we fix the virstion to `0.20.0`.
On linux and mac, simply you can do like below!

```bash
./installers/install_bazel.sh
```

Also we need python and numpy. Currently we tested on python3. Our suggestion is using `pipenv`.

```bash
pipenv --three
pip install numpy
```

Or you can set environment variable `PYTHON_BIN_PATH` to `/path/to/python/`.

### For Windows developers

For only window developers, you have to do more. You have to download llvm from [llvm.org](http://llvm.org/builds/) also. Because you need `clang-cl` for compiler. And then to change default compiler for bazel, you have to set `USE_CLANG_CL` to `1` and `BAZEL_LLVM` to `/path/to/llvm`. This feature is a quite new, so you need a recent released one. We tested on `0.20.0`.

## How to build

```bash
bazel build //felicia/...
```

### For Macos developers

Currently, in bazel, `--cpu` option's default value is `darwin`, which doesn't work on our project(If your cpu is 64bit.). You have to manually set `darwin_x86_64` on every command like below.

```bash
bazel build --cpu darwin_x86_64 //felicia/...
```

Also in order to leave debugging symbol on mac, you have to add config `apple_debug` like below. If you aren't working on mac, it's enough to set `-c dbg`.

```bash
bazel build --cpu darwin_x86_64 --config apple_debug //felicia/...
```

## How to test

```bash
bazel test //felicia/...
```

## How to set up on Docker

```bash
docker build . -t felicia -f docker/Dockerfile.ubuntu
```

You can validate your work on docker with below.

```bash
python3 scripts/docker_exec.py bazel build //felicia/...
```

## For Users

Follow this link to check [examples](felicia/examples)

## For Developers

After you write the any BUILD, or *.bzl, don't forget to run buildifier!

```bash
bazel run //:buildifier
```
