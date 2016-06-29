# Multiverso Python/Theano/Lasagne Binding


## Introduction
Multiverso is a parameter server framework for distributed machine learning. This package can leverage multiple machines and GPUs to speed up the python programs.


## Installation

1. (For GPU support only) Install CUDA, cuDNN according to this [guide](https://github.com/Microsoft/fb.resnet.torch/blob/multiverso/INSTALL.md). You just need finish the steps before [Install Torch](https://github.com/Microsoft/fb.resnet.torch/blob/multiverso/INSTALL.md#install-torch).
1. Install the multiverso
    * On linux: Please follow the [README](../../README.md#build) to build and install multiverso.
    * On windows: You need MSBuild.exe installed and make sure your system can find it in the $PATH. Then you should run [build_dll.bat](../../src/build_dll.bat) to build the .dll file and install the .dll. There isn't auto-installer for windows now, so you have to copy the .dll to either system $PATH or the multiverso package folder.
1. Install python binding with the command `sudo python setup.py install`


## Run Unit Tests
```
nosetests
```


## Documentation
* [Tutorial](./docs/TUTORIAL.md)
* Api documents are written as docstrings in the python source code.
* [Benchmark](./docs/BENCHMARK.md)
