# Multiverso Torch/Lua Binding

## Introduction
Multiverso is a parameter server framework for distributed machine learning.
This package can leverage multiple machines and GPUs to speed up the torch
programs.

## Requirements
Build multiverso successfully by following the [README > build](../../README.md#build).

## Installation

**NOTE**: Before installation, you need to make sure have `libmultiverso.so`
build successfully according to [Requirements](README.md#Requirements).

```
make install
```
or
```
luarocks make
```

## Unit Tests
```
make test
```
or

```
luajit test.lua
```

## Documentation

- [Tutorial](https://github.com/Microsoft/multiverso/wiki/Integrate-multiverso-into-torch-project)
- [API](https://github.com/Microsoft/multiverso/wiki/Multiverso-Torch-Binding-API)
- [Benchmark](https://github.com/Microsoft/multiverso/wiki/Multiverso-Torch-Binding-Benchmark)
