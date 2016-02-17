#include <iostream>

#include <multiverso/multiverso.h>
#include <multiverso/util/log.h>

#include <multiverso/table/array_table.h>
#include <multiverso/table/kv_table.h>
#include <multiverso/net.h>

using namespace multiverso;

void TestKV() {
  Log::Info("Test KV map \n");
  // ----------------------------------------------------------------------- //
  // this is a demo of distributed hash table to show how to use the multiverso
  // ----------------------------------------------------------------------- //

  // 1. Start the Multiverso engine ---------------------------------------- //
  MultiversoInit();

  // 2. To create the shared table ----------------------------------------- //

  // TODO(feiga): This table must be create at both worker and server endpoint 
  // simultaneously, since they should share same type and same created order
  // (same order means same table id). So it's better to create them with some 
  // specific creator, instead of current way

  // TODO(feiga): should add some if statesment
  // if the node is worker, then create a worker cache table
  KVWorkerTable<int, int>* dht = new KVWorkerTable<int, int>();
  // if the node is server, then create a server storage table
  KVServerTable<int, int>* server_dht = new KVServerTable<int, int>();

  MultiversoBarrier();

  // 3. User program ------------------------------------------------------- //

  // all this interface is related with the KVWorkerTable
  // the data structure and the interface can be both defined by users
  // We also provides several common implementations
  // For specific program, user-defined table may provides better performance

  // access the local cache
  std::unordered_map<int, int>& kv = dht->raw();

  // The Get/Add are sync operation, when the function call returns, we already
  // Get from server, or the server has Added the update

  // Get from the server
  dht->Get(0);
  // Check the result. Since no one added, this should be 0
  Log::Info("Get 0 from kv server: result = %d\n", kv[0]);

  // Add 1 to the server
  dht->Add(0, 1);

  // Check the result. Since just added one, this should be 1
  dht->Get(0);

  Log::Info("Get 0 from kv server after add 1: result = %d\n", kv[0]);

  // 4. Shutdown the Multiverso engine. ------------------------------------ //
  MultiversoShutDown();
}

void TestArray() {
  Log::Info("Test Array \n");

  MultiversoInit();
  
  ArrayWorker<float>* shared_array = new ArrayWorker<float>(10);
  ArrayServer<float>* server_array = new ArrayServer<float>(10);

  MultiversoBarrier();
  Log::Info("Create tables OK\n");

  // std::vector<float>& vec = shared_array->raw();

  // shared_array->Get();
  float data[10];
  shared_array->Get(data, 10);

  Log::Info("Get OK\n");

  for (int i = 0; i < 10; ++i) std::cout << data[i] << " "; std::cout << std::endl;

  std::vector<float> delta(10);
  for (int i = 0; i < 10; ++i) delta[i] = static_cast<float>(i);

  shared_array->Add(delta.data(), 10);

  Log::Info("Add OK\n");

  shared_array->Get(data, 10);

  for (int i = 0; i < 10; ++i) std::cout << data[i] << " "; std::cout << std::endl;

  MultiversoShutDown();
}


void TestNet() {
  NetInterface* net = NetInterface::Get();
  net->Init();

  char* hi = "hello, world";
  if (net->rank() == 0) {
    MessagePtr msg = std::make_unique<Message>();
    msg->set_src(0);
    msg->set_dst(1);
    msg->Push(Blob(hi, 13));
    net->Send(msg);
    Log::Info("rank 0 send\n");
  } else if (net->rank() == 1) {
    MessagePtr msg = std::make_unique<Message>();
    net->Recv(&msg);
    Log::Info("rank 1 recv\n");
    CHECK(strcmp(msg->data()[0].data(), hi) == 0);
  }

  net->Finalize();
}

int main(int argc, char* argv[]) {
  if (argc == 2) { 
    if (strcmp(argv[1], "kv") == 0) TestKV();
    else if (strcmp(argv[1], "array") == 0) TestArray();
    else if (strcmp(argv[1], "net") == 0) TestNet();
    else CHECK(false);
  } else {
    TestArray();
  }
  return 0;
}