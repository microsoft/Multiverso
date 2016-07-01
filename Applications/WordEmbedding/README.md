Distributed Word Embedding
==========

The Distributed Word Embedding tool is a parallelization of the Word2Vec algorithm on top of our DMTK parameter server. It provides an efficient "scaling to industry size" solution for word embedding.

For more details about parameters setting and performance, please view our [Wiki](https://github.com/Microsoft/multiverso/wiki/Word-Embedding) and our website [DMTK](http://www.dmtk.io).

#Why Distributed Word Embedding?

1. For traning a large dataset.

   The DMTK parameter server stores the parameters in a distributed way, which means that each machine just holds a partition of the entire parameter set. This allows the entire embedding vector to be very large. For example, in experiment on the ClueWeb data, the vocabulary size is 21 Million, and the parameter size reaches 6 Billion, which is the largest word embedding model ever reported in the literature,as far as we know. 

2. For high quality word embedding. 

   You can view the performance of Distributed Word Embedding in [Wiki](https://github.com/Microsoft/multiverso/wiki/Word-Embedding).

3. For less traning time.

   Large dataset need long traning time. You can accelerate process of training by use multi-machines.

#How Distributed Word Embedding work?

   The DWE tool runs in the following manner:

   On the client side (running on multiple nodes): three local training steps are executed repeatedly: 

      1). Get the latest parameters from the DMTK parameter server

      2). Run the CBOW/Skip-gram algorithm to generate updates to the current parameters

      3). Send the parameter updates to the DMTK parameter server

   On the server side, the DMTK parameter server acts as below:

      1). Pack the requested parameters and send them to clients

      2). Aggregate parameter updates from different clients and merge them into the global parameters

#Linux Installation

1. cmake ./CMakeLists.txt

2. make

# Windows Installation

1. Get and build the DMTK Framework [multiverso](https://github.com/Microsoft/multiverso.git).

2. Open Multiverso.sln, change configuration and platform to Release and x64 of WordEmbedding(default setting), set the ```include``` and ```lib``` path of multiverso in project property.

3. Enable openmp 2.0 support.

   To set this **compiler** option in the Visual Studio development environment
  
   1)Open the **project's Property** Pages dialog box.
  
   2)Expand the **Configuration Properties** node.
  
   3)Expand the **C/C++** node.
  
   4)Select the **Language** property page.
  
   5)Modify the **OpenMP Support property** to "yes".
   
4. Build the solution.
