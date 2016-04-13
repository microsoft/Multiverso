multiverso
==========
Multiverso-initial will be deprecated. please refer to the master branch to use latest version.

Multiverso is a parameter server based framework for training machine learning models on big data with numbers of machines. It is currently a standard C++ library and provides a series of friendly programming interfaces. With such easy-to-use APIs, machine learning researchers and practitioners do not need to worry about the system routine issues such as distributed model storage and operation, inter-process and inter-thread communication, multi-threading management, and so on.
Instead, they are able to focus on the core machine learning logics: data, model, and training.

For more details, please view our website [http://www.dmtk.io](http://www.dmtk.io).

Build
----------

**Linux** (Tested on Ubuntu 12.04)

0. Run ```cd third_party``` 
1. Run ```./install.sh``` to install the dependence.
2. Run ```make all -j4``` to build the multiverso.

**Cmake**

0. Run ``` cd third_party ``` 
1. Run ``` ./install.sh ``` to install the dependence.
1. Run ``` cd .. ```
1. Run ``` mkdir build ```
2. Run ``` cd build ```
3. Run ``` cmake .. ```
4. Run ``` make ```

**Windows**

For windows users, please refer to README in windows folder.


Related Projects
----------

Current distributed systems based on multiverso:

* [lightlda](https://github.com/Microsoft/lightlda): Scalable, fast, lightweight system for large scale topic modeling
* [distributed_word_embedding](https://github.com/Microsoft/distributed_word_embedding) Distributed system for word embedding
* [distributed_skipgram_mixture](https://github.com/Microsoft/distributed_skipgram_mixture) Distributed skipgram mixture for multi-sense word embedding

