# Multiverso Torch/Lua Binding

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
