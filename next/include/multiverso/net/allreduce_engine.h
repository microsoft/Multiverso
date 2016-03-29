#ifndef MULTIVERSO_NET_ALLREDUCE_ENGINE_H_
#define MULTIVERSO_NET_ALLREDUCE_ENGINE_H_

#ifdef MULTIVERSO_USE_ZMQ

#include <vector>

namespace multiverso {

typedef unsigned char byte;

class AllreduceNetWrapper;

typedef void (ReduceFunction)(const byte *src, byte *dst, int len);

class BruckMap {
public:
  int k;
  std::vector<int> in_ranks;
  std::vector<int> out_ranks;
  BruckMap();
  BruckMap(int n);
  static BruckMap Construct(int rank, int worldSize);
};

enum RecursiveHalvingNodeType {
  Normal, ReciveNeighbor, SendNeighbor
};

class RecursiveHalvingMap {
public:
  int k;
  RecursiveHalvingNodeType type;
  int neighbor;
  std::vector<int> ranks;
  std::vector<int> send_block_start;
  std::vector<int> send_block_len;
  std::vector<int> recv_block_start;
  std::vector<int> recv_block_len;
  RecursiveHalvingMap();
  RecursiveHalvingMap(RecursiveHalvingNodeType _type, int n);
  static RecursiveHalvingMap Construct(int rank, int world_size);
};

class AllreduceEngine {
public:
  AllreduceEngine() {}

  void Init(const AllreduceNetWrapper* linkers);

  ~AllreduceEngine();

  inline int Rank();

  inline int WorldSize();

  void Allreduce(byte* input, int input_size, int type_size, byte* output, ReduceFunction reducer);

  void AllreduceByAllGather(byte* input, int input_size, int type_size, byte* output, ReduceFunction reducer);

  //use bruck Algorithm.
  //Thakur, Rajeev, Rolf Rabenseifner, and William Gropp. 
  //"Optimization of collective communication operations in MPICH." International Journal of High Performance Computing Applications 19.1 (2005): 49-66.
  void Allgather(byte* input, int send_size, byte* output);

  void Allgather(byte* input, int all_size, int* block_start, int* block_len, byte* output);

  void ReduceScatter(byte* input, int input_size, int type_size, int* block_start, int* block_len, byte* output, ReduceFunction reducer);

private:
  int world_size_;
  int rank_;
  const AllreduceNetWrapper *linkers_;
  BruckMap bruck_map_;
  RecursiveHalvingMap recursive_halving_map_;
  int* block_start_;
  int* block_len_;
  byte* buffer_;
  int buffer_size_;
};

inline int AllreduceEngine::Rank() {
  return rank_;
}

inline int AllreduceEngine::WorldSize() {
  return world_size_;
}

}

#endif // MULTIVERSO_USE_ZMQ

#endif //MULTIVERSO_NET_ALLREDUCE_ENGINE_H_