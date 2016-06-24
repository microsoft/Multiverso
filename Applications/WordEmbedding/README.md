Distributed Word Embedding
==========

The Distributed Word Embedding tool is a parallelization of the Word2Vec algorithm on top of our DMTK parameter server. It provides an efficient "scaling to industry size" solution for word embedding.

For more details, please view our website [http://www.dmtk.io](http://www.dmtk.io).

#Linux Installation

cmake CMakeLists.txt

make

# Windows Installation

1. Get and build the DMTK Framework [multiverso](https://github.com/Microsoft/multiverso.git).

2. Open Multiverso.sln, change configuration and platform to Release and x64 of WordEmbedding(default setting), set the ```include``` and ```lib``` path of multiverso in project property.

3. Enable openmp 2.0 support.

4. Build the solution.
