# Installation

## On linux
Please follow the [README](../../README.md#build) to build and install multiverso.

## On windows
You need MSBuild.exe installed and make sure your system can find it in the $PATH. Then you should run [build_dll.bat](../../src/build_dll.bat) to build the .dll file and install the .dll. Multiverso doesn't have auto-installer for windows now, you have to copy the .dll to either system $PATH or the multiverso package folder.


# Run Unit Tests
```
nosetests
```


# Documentation
* [Experiments](./docs/EXPERIMENTS.md)
* [Tutorial](./docs/TUTORIAL.md)
* Api documents are written as docstrings in the python source code.
