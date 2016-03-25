#ifndef MULTIVERSO_NET_ALLREDUCE_ENGINE_H_
#define MULTIVERSO_NET_ALLREDUCE_ENGINE_H_

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
  static void Init(const AllreduceNetWrapper* linkers);

  static void Dispose();

  static inline int Rank();

  static inline int WorldSize();

  static void Allreduce(byte* input, int input_size, int type_size, byte* output, ReduceFunction reducer);

  static void AllreduceByAllGather(byte* input, int input_size, int type_size, byte* output, ReduceFunction reducer);

  //use bruck Algorithm.
  //Thakur, Rajeev, Rolf Rabenseifner, and William Gropp. 
  //"Optimization of collective communication operations in MPICH." International Journal of High Performance Computing Applications 19.1 (2005): 49-66.
  static void Allgather(byte* input, int send_size, int all_size, byte* output);

  static void Allgather(byte* input, int all_size, int* block_start, int* block_len, byte* output);

  static void ReduceScatter(byte* input, int input_size, int type_size, int* block_start, int* block_len, byte* output, ReduceFunction reducer);

private:
  static int world_size_;
  static int rank_;
  static const AllreduceNetWrapper *linkers_;
  static BruckMap bruck_map_;
  static RecursiveHalvingMap recursive_halving_map_;
  static int* block_start_;
  static int* block_len_;
  static byte* buffer_;
  static int buffer_size_;
};

inline int AllreduceEngine::Rank() {
  return rank_;
}

inline int AllreduceEngine::WorldSize() {
  return world_size_;
}

}
#endif //MULTIVERSO_NET_ALLREDUCE_ENGINE_H_