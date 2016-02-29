#include <iostream>
#include <thread>
#include <random>
#include <chrono>

#include <mpi.h>

#include <multiverso/multiverso.h>
#include <multiverso/net.h>
#include <multiverso/util/log.h>
#include <multiverso/util/net_util.h>

#include <multiverso/table/smooth_array_table.h>
#include <multiverso/table/array_table.h>
#include <multiverso/table/kv_table.h>
using namespace multiverso;

void TestKV(int argc, char* argv[]) {
  Log::Info("Test KV map \n");
  // ----------------------------------------------------------------------- //
  // this is a demo of distributed hash table to show how to use the multiverso
  // ----------------------------------------------------------------------- //

  // 1. Start the Multiverso engine ---------------------------------------- //
  MultiversoInit(&argc, argv);

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

void TestArray(int argc, char* argv[]) {
  Log::Info("Test Array \n");

  MultiversoInit(&argc, argv);
  
  ArrayWorker<float>* shared_array = new ArrayWorker<float>(1000000);
  ArrayServer<float>* server_array = new ArrayServer<float>(1000000);

  MultiversoBarrier();
  Log::Info("Create tables OK\n");

  int iter = 1000;
  if (argc == 3) iter = atoi(argv[2]);

  for (int i = 0; i < iter; ++i) {
  // std::vector<float>& vec = shared_array->raw();

  // shared_array->Get();
    float* data = new float[1000000];

    std::vector<float> delta(1000000);
    for (int i = 0; i < 1000000; ++i) 
      delta[i] = static_cast<float>(i);

    shared_array->Add(delta.data(), 1000000);

    Log::Info("Rank %d Add OK\n", MultiversoRank());

    shared_array->Get(data, 1000000);
    Log::Info("Rank %d Get OK\n", MultiversoRank());
    for (int i = 0; i < 10; ++i) 
      std::cout << data[i] << " "; std::cout << std::endl;
    MultiversoBarrier();

  }
  MultiversoShutDown();
}

void TestMomentum(int argc, char* argv[]) {
	Log::Info("Test smooth_gradient table \n");


	Log::ResetLogLevel(LogLevel::Debug);
	MultiversoInit();

	SmoothArrayWorker<float>* shared_array = new SmoothArrayWorker<float>(10);
	SmoothArrayServer<float>* server_array = new SmoothArrayServer<float>(10);

	MultiversoBarrier();
	Log::Info("Create tables OK\n");

	while (true){
		// std::vector<float>& vec = shared_array->raw();

		// shared_array->Get();
		float data[10];

		std::vector<float> delta(10);
		for (int i = 0; i < 10; ++i)
			delta[i] = static_cast<float>(i+1);

		shared_array->Add(delta.data(), 10, 0.5f);

		Log::Info("Rank %d Add OK\n", MultiversoRank());

		shared_array->Get(data, 10);
		Log::Info("Rank %d Get OK\n", MultiversoRank());
		for (int i = 0; i < 10; ++i)
			std::cout << data[i] << " "; std::cout << std::endl;
		MultiversoBarrier();

	}
	MultiversoShutDown();
}

#define ARRAY_SIZE 4683776
void TestMultipleThread(int argc, char* argv[])
{
	Log::Info("Test Multiple threads \n");
	std::mt19937_64 eng{ std::random_device{}() };  
	std::uniform_int_distribution<> dist{ 5, 10000 };
	std::this_thread::sleep_for(std::chrono::milliseconds{ dist(eng) });
	//Log::ResetLogLevel(LogLevel::Debug);
	MultiversoInit(&argc, argv);

	ArrayWorker<float>* shared_array = new ArrayWorker<float>(ARRAY_SIZE);
	ArrayServer<float>* server_array = new ArrayServer<float>(ARRAY_SIZE);
	std::thread* m_prefetchThread = nullptr;
	MultiversoBarrier();
	Log::Info("Create tables OK\n");

	std::vector<float> delta(ARRAY_SIZE);
	while (true){
		if (m_prefetchThread != nullptr && m_prefetchThread->joinable())
		{
			m_prefetchThread->join();
			delete m_prefetchThread;
			m_prefetchThread = nullptr;
		}

		std::fill(delta.begin(), delta.end(), 0);
		for (int i = 0; i < ARRAY_SIZE; ++i)
		{
		std::mt19937_64 eng{ std::random_device{}() };
			std::uniform_real_distribution<float> dist{ -1, 1 };
			delta[i] = dist(eng);
		}
		m_prefetchThread = new std::thread([&](){
			
			//std::mt19937_64 eng{ std::random_device{}() };  
			//std::uniform_int_distribution<> dist{ 50, 500 };
			//std::this_thread::sleep_for(std::chrono::milliseconds{ dist(eng) });
			shared_array->Add(delta.data(), ARRAY_SIZE);
			shared_array->Get(delta.data(), ARRAY_SIZE);
			Log::Info("Rank %d Get OK\n", MultiversoRank());
			for (int i = 0; i < 10; ++i)
				std::cout << delta[i] << " "; std::cout << std::endl;
		});

		//shared_array->Get(data, 10);
		MultiversoBarrier();

	}
	MultiversoShutDown();
}


void TestNet(int argc, char* argv[]) {
  NetInterface* net = NetInterface::Get();
  net->Init(&argc, argv);

  char* hi1 = "hello, world";
  char* hi2 = "hello, c++";
  char* hi3 = "hello, multiverso";
  if (net->rank() == 0) {
    MessagePtr msg(new Message());// = std::make_unique<Message>();
    msg->set_src(0);
    msg->set_dst(1);
    msg->Push(Blob(hi1, 13));
    msg->Push(Blob(hi2, 11));
    msg->Push(Blob(hi3, 18));
    while(net->Send(msg) == 0) ;
    Log::Info("rank 0 send\n");
    Log::Info("Hi = %s\n", msg->data()[0].data());

    msg.reset(new Message());
    while (net->Recv(&msg) == 0) {
      Log::Info("recv return 0\n");
    }
    Log::Info("rank 0 recv\n");
    // CHECK(strcmp(msg->data()[0].data(), hi) == 0);
    std::vector<Blob> recv_data = msg->data();
    CHECK(recv_data.size() == 3);
    for (int i = 0; i < msg->size(); ++i) {
      Log::Info("%s\n", recv_data[i].data());
    };
  } else if (net->rank() == 1) {
    MessagePtr msg(new Message());// = std::make_unique<Message>();
    while (net->Recv(&msg) == 0) {
      Log::Info("recv return 0\n");
    }
    Log::Info("rank 1 recv\n");
    // CHECK(strcmp(msg->data()[0].data(), hi) == 0);
    std::vector<Blob> recv_data = msg->data();
    CHECK(recv_data.size() == 3);
    for (int i = 0; i < msg->size(); ++i) {
      Log::Info("%s\n", recv_data[i].data());
    }

    msg.reset(new Message());
    msg->set_src(1);
    msg->set_dst(0);
    msg->Push(Blob(hi1, 13));
    msg->Push(Blob(hi2, 11));
    msg->Push(Blob(hi3, 18));
    while (net->Send(msg) == 0);
    Log::Info("rank 0 send\n");
    Log::Info("Hi = %s\n", msg->data()[0].data());
  }

  net->Finalize();
}

void TestIP() {
  std::unordered_set<std::string> ip_list;
  // net::GetLocalIPAddress(&ip_list);
  for (auto ip : ip_list) Log::Info("%s\n", ip.c_str());
}

void TestNoNet(int argc, char* argv[]) {  
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);

  MPI_Barrier(MPI_COMM_WORLD);
  MultiversoInit(&argc, argv);

  ArrayWorker<float>* shared_array = new ArrayWorker<float>(ARRAY_SIZE);
  ArrayServer<float>* server_array = new ArrayServer<float>(ARRAY_SIZE);
  std::thread* m_prefetchThread = nullptr;
  MultiversoBarrier();
  Log::Info("Create tables OK\n");

  std::vector<float> delta(ARRAY_SIZE);
  while (true){
    if (m_prefetchThread != nullptr && m_prefetchThread->joinable())
    {
      m_prefetchThread->join();
      delete m_prefetchThread;
      m_prefetchThread = nullptr;
    }

    std::fill(delta.begin(), delta.end(), 0);
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      std::mt19937_64 eng{ std::random_device{}() };
      std::uniform_real_distribution<float> dist{ -1, 1 };
      delta[i] = dist(eng);
    }
    m_prefetchThread = new std::thread([&](){

      //std::mt19937_64 eng{ std::random_device{}() };  
      //std::uniform_int_distribution<> dist{ 50, 500 };
      //std::this_thread::sleep_for(std::chrono::milliseconds{ dist(eng) });
      shared_array->Add(delta.data(), ARRAY_SIZE);
      shared_array->Get(delta.data(), ARRAY_SIZE);
      Log::Info("Rank %d Get OK\n", MultiversoRank());
      for (int i = 0; i < 10; ++i)
        std::cout << delta[i] << " "; std::cout << std::endl;
    });

    //shared_array->Get(data, 10);
    MultiversoBarrier();

  }
  MultiversoShutDown();
}

int main(int argc, char* argv[]) {
  Log::ResetLogLevel(LogLevel::Debug);
  if (argc == 2) { 
    if (strcmp(argv[1], "kv") == 0) TestKV(argc, argv);
    else if (strcmp(argv[1], "array") == 0) TestArray(argc, argv);
    else if (strcmp(argv[1], "net") == 0) TestNet(argc, argv);
    else if (strcmp(argv[1], "ip") == 0) TestIP();
	else if (strcmp(argv[1], "momentum") == 0) TestMomentum(argc, argv);
	else if (strcmp(argv[1], "threads") == 0) TestMultipleThread(argc, argv);
  else if (strcmp(argv[1], "nonet") == 0) TestNoNet(argc, argv);
    else CHECK(false);
  } 
  // argc == 4 is for zeromq test, with two extra arguments: machinefile, port
  else if (argc == 4) {
    if (strcmp(argv[3], "kv") == 0) TestKV(argc, argv);
    else if (strcmp(argv[3], "array") == 0) TestArray(argc, argv);
    else if (strcmp(argv[3], "net") == 0) TestNet(argc, argv);
    else if (strcmp(argv[3], "ip") == 0) TestIP();
  } else {
    TestArray(argc, argv);
  }
  return 0;
}
