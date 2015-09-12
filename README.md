multiverso
==========

Multiverso is a parameter server based framework for training machine learning models on big data with numbers of machines. It is currently a standard C++ library and provides a series of friendly programming interfaces. With such easy-to-use APIs, machine learning researchers and practitioners do not need to worry about the system routine issues such as distributed model storage and operation, inter-process and inter-thread communication, multi-threading management, and so on.
Instead, they are able to focus on the core machine learning logics: data, model, and training.

For more details, please view our [website](www.dmtk.io).

Build
----------

**Linux** (Tested on Ubuntu 12.04)

1. Run ```./third_party/install.sh``` to download the Zeromq and MPICH2. (If you have installed before, just ignore and modified related path in Makefile)
2. Run ```make all -j4``` to build the multiverso.

**Windows**

For windows users, please refer to README in windows folder.


Related Porjests
----------

Current distributed systems based on multiverso:

* [lightlda](https://github.com/Microsoft/lightlda): Scalable, fast, lightweight system for large scale topic modeling
* [distributed_word_embedding](https://github.com/Microsoft/distributed_word_embedding)
* [distributed_skipgram_mixture](https://github.com/Microsoft/distributed_skipgram_mixture)

