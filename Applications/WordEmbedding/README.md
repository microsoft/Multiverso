Distributed Word Embedding
==========

The Distributed Word Embedding tool is a parallelization of the Word2Vec algorithm on top of our DMTK parameter server. It provides an efficient "scaling to industry size" solution for word embedding.

For more details, please view our website [http://www.dmtk.io](http://www.dmtk.io).

#Linux Installation

1. cmake ./CMakeLists.txt

2. make

# Windows Installation

1. Get and build the DMTK Framework [multiverso](https://github.com/Microsoft/multiverso.git).

2. Open Multiverso.sln, change configuration and platform to Release and x64 of WordEmbedding(default setting), set the ```include``` and ```lib``` path of multiverso in project property.

3. Enable openmp 2.0 support.

   To set this compiler option in the Visual Studio development environment
  
   1)Open the project's Property Pages dialog box. For details, see How to: Open Project Property Pages.
  
   2)Expand the Configuration Properties node.
  
   3)Expand the C/C++ node.
  
   4)Select the Language property page.
  
   5)Modify the OpenMP Support property.
   
4. Build the solution.
