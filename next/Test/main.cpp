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
#include <multiverso/table/matrix_table.h>
#include <MPIWrapper.h>


using namespace multiverso;

void TestKV(int argc, char* argv[]) {
  Log::Info("Test KV map \n");
  // ----------------------------------------------------------------------- //
  // this is a demo of distributed hash table to show how to use the multiverso
  // ----------------------------------------------------------------------- //

  // 1. Start the Multiverso engine ---------------------------------------- //
  MV_Init(&argc, argv);

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
  Log::Info("table name %s", server_dht->name().c_str());

  MV_Barrier();

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
  MV_ShutDown();
}

void TestArray(int argc, char* argv[]) {
  Log::Info("Test Array \n");

  MV_Init(&argc, argv);
  
  ArrayWorker<float>* shared_array = new ArrayWorker<float>(1000000);
  ArrayServer<float>* server_array = new ArrayServer<float>(1000000);

  MV_Barrier();
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

    Log::Info("Rank %d Add OK\n", MV_Rank());

    shared_array->Get(data, 1000000);
    Log::Info("Rank %d Get OK\n", MV_Rank());
    for (int i = 0; i < 10; ++i) 
      std::cout << data[i] << " "; std::cout << std::endl;
    MV_Barrier();

  }
  MV_ShutDown();
}

void TestMomentum(int argc, char* argv[]) {
	Log::Info("Test smooth_gradient table \n");


	Log::ResetLogLevel(LogLevel::Debug);
	MV_Init();

	SmoothArrayWorker<float>* shared_array = new SmoothArrayWorker<float>(10);
	SmoothArrayServer<float>* server_array = new SmoothArrayServer<float>(10);

	MV_Barrier();
	Log::Info("Create tables OK\n");

	while (true){
		// std::vector<float>& vec = shared_array->raw();

		// shared_array->Get();
		float data[10];

		std::vector<float> delta(10);
		for (int i = 0; i < 10; ++i)
			delta[i] = static_cast<float>(i+1);

		shared_array->Add(delta.data(), 10, 0.5f);

		Log::Info("Rank %d Add OK\n", MV_Rank());

		shared_array->Get(data, 10);
		Log::Info("Rank %d Get OK\n", MV_Rank());
		for (int i = 0; i < 10; ++i)
			std::cout << data[i] << " "; std::cout << std::endl;
		MV_Barrier();

	}
	MV_ShutDown();
}

#define ARRAY_SIZE 4683776
void TestMultipleThread(int argc, char* argv[])
{
	Microsoft::MSR::CNTK::MPIWrapper *g_mpi = new Microsoft::MSR::CNTK::MPIWrapper();
	Log::Info("Test Multiple threads \n");
	std::mt19937_64 eng{ std::random_device{}() };  
	std::uniform_int_distribution<> dist{ 5, 10000 };
	std::this_thread::sleep_for(std::chrono::milliseconds{ dist(eng) });
	//Log::ResetLogLevel(LogLevel::Debug);
	MV_Init(&argc, argv);

	ArrayWorker<float>* shared_array = new ArrayWorker<float>(ARRAY_SIZE);
	ArrayServer<float>* server_array = new ArrayServer<float>(ARRAY_SIZE);
	std::thread* m_prefetchThread = nullptr;
	MV_Barrier();
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
			Log::Info("Rank %d Get OK\n", MV_Rank());
			for (int i = 0; i < 10; ++i)
				std::cout << delta[i] << " "; std::cout << std::endl;
		});

		//shared_array->Get(data, 10);
		MV_Barrier();

	}
	MV_ShutDown();
}


void TestNet(int argc, char* argv[]) {
  NetInterface* net = NetInterface::Get();
  net->Init(&argc, argv);

  const char* chi1 = std::string("hello, world").c_str();
  const char* chi2 = std::string("hello, c++").c_str();
  const char* chi3 = std::string("hello, multiverso").c_str();
  char* hi1 = new char[14];
  strcpy(hi1, chi1);
  char* hi2 = new char[12];
  strcpy(hi2, chi2);
  char* hi3 = new char[19];
  strcpy(hi3, chi3);
  if (net->rank() == 0) {
    for (int rank = 1; rank < net->size(); ++rank) {
      MessagePtr msg(new Message());// = std::make_unique<Message>();
      msg->set_src(0);
      msg->set_dst(rank);
      msg->Push(Blob(hi1, 13));
      msg->Push(Blob(hi2, 11));
      msg->Push(Blob(hi3, 18));
      for (int i = 0; i < msg->size(); ++i) {
        Log::Info("In Send: %s\n", msg->data()[i].data());
      };
      while (net->Send(msg) == 0) ;
      Log::Info("rank 0 send\n");
    }

    for (int i = 1; i < net->size(); ++i) {
      MessagePtr msg(new Message());
      msg.reset(new Message());
      while (net->Recv(&msg) == 0) {
        // Log::Info("recv return 0\n");
      }
      Log::Info("rank 0 recv\n");
      // CHECK(strcmp(msg->data()[0].data(), hi) == 0);
      std::vector<Blob> recv_data = msg->data();
      CHECK(recv_data.size() == 3);
      for (int i = 0; i < msg->size(); ++i) {
        Log::Info("recv from srv %d: %s\n", msg->src(), recv_data[i].data());
      };
    }
  } else {// other rank
    MessagePtr msg(new Message());// = std::make_unique<Message>();
    while (net->Recv(&msg) == 0) {
      // Log::Info("recv return 0\n");
    }
    Log::Info("rank %d recv\n", net->rank());
    // CHECK(strcmp(msg->data()[0].data(), hi) == 0);
    std::vector<Blob>& recv_data = msg->data();
    CHECK(recv_data.size() == 3);
    for (int i = 0; i < msg->size(); ++i) {
      Log::Info("%s\n", recv_data[i].data());
    }

    msg.reset(new Message());
    msg->set_src(net->rank());
    msg->set_dst(0);
    msg->Push(Blob(hi1, 13));
    msg->Push(Blob(hi2, 11));
    msg->Push(Blob(hi3, 18));
    while (net->Send(msg) == 0) ;
    Log::Info("rank %d send\n", net->rank());
  }
  // while (!net->Test()) {
  //  // wait all message process finished
  // }

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
  MV_Init(&argc, argv);

  ArrayWorker<float>* shared_array = new ArrayWorker<float>(ARRAY_SIZE);
  ArrayServer<float>* server_array = new ArrayServer<float>(ARRAY_SIZE);
  std::thread* m_prefetchThread = nullptr;
  MV_Barrier();
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
      Log::Info("Rank %d Get OK\n", MV_Rank());
      for (int i = 0; i < 10; ++i)
        std::cout << delta[i] << " "; std::cout << std::endl;
    });

    //shared_array->Get(data, 10);
    MV_Barrier();

  }
  MV_ShutDown();
}

void TestMatrix(int argc, char* argv[]){
	Log::Info("Test Matrix\n");

	MV_Init(&argc, argv);

	int num_row = 11, num_col = 10;
	int size = num_row * num_col;

  MatrixWorkerTable<int>* worker_table = 
    static_cast<MatrixWorkerTable<int>*>(MV_CreateTable<int>("matrix", { &num_row, &num_col }));  //new implementation
    //static_cast<MatrixWorkerTable<int>*>((new MatrixTableHelper<int>(num_row, num_col))->CreateTable()); //older one

  if (worker_table == nullptr){ //should have more if statement to avoid nullptr in using worker_table
    Log::Debug("rank %d has no worker\n", MV_Rank());
  }

	//MatrixWorkerTable<int>* worker_table = new MatrixWorkerTable<int>(num_row, num_col);
	//MatrixServerTable<int>* server_table = new MatrixServerTable<int>(num_row, num_col);

	MV_Barrier();

	std::vector<int> v = { 0, 1, 5 ,10};

	// test data
	std::vector<int> delta(size);
	for (int i = 0; i < size; ++i)
		delta[i] = i;

	int * data = new int[size];

	// worker_table->Add(v, delta.data()); //add row 0,1,5,10
	worker_table->Add(delta.data(), size); //add all

	worker_table->Get(data, size); //get all
	MV_Barrier();

	printf("----------------------------\n");
	for (int i = 0; i < num_row; ++i){
		printf("rank %d, row %d: ", MV_Rank(), i);
		for (int j = 0; j < num_col; ++j)
			printf("%d ", data[i * num_col + j]);
		printf("\n");
	}
	MV_Barrier();

	//test data_vec
	std::vector<int*> data_rows = { &data[0], &data[num_col], &data[5 * num_col], &data[10*num_col] };
	std::vector<int*> delta_rows = { &delta[0], &delta[num_col], &delta[5 * num_col], &delta[10 * num_col] };

  worker_table->Add(v, delta_rows, num_col);
  worker_table->Get(v, data_rows, num_col);
  MV_Barrier();

	printf("----------------------------\n");
	for (int i = 0; i < num_row; ++i){
		printf("rank %d, row %d: ", MV_Rank(), i);
		for (int j = 0; j < num_col; ++j)
			printf("%d ", data[i * num_col + j]);
		printf("\n");
	}
	MV_Barrier();
	
	MV_ShutDown();
}

void TestCheckPoint(int argc, char* argv[], bool restore){
  Log::Info("Test CheckPoint\n");

  MV_Init(&argc, argv, Role::All, restore);

  int num_row = 11, num_col = 10;
  int size = num_row * num_col;

  MatrixWorkerTable<int>* worker_table =
    static_cast<MatrixWorkerTable<int>*>((new MatrixTableHelper<int>(num_row, num_col))->CreateTable());
  //MatrixWorkerTable<int>* worker_table = new MatrixWorkerTable<int>(num_row, num_col);
  //MatrixServerTable<int>* server_table = new MatrixServerTable<int>(num_row, num_col);
  //if restore = true, will restore server data and return the next iter number of last dump file
  //else do nothing and return 0
  if (worker_table == nullptr) {
    //no worker in this node
  }
  int begin_iter = MV_LoadTable("./serverTable_");
  MV_Barrier();//won't dump data without parameters

  std::vector<int> delta(size);
  for (int i = 0; i < size; ++i)
    delta[i] = i;
  int * data = new int[size];

  Log::Debug("rank %d start from iteration %d\n", MV_Rank(), begin_iter);

  for (int i = begin_iter; i < 50; ++i){
    worker_table->Add(delta.data(), size);
    MV_Barrier(i); //dump table data with iteration i each k iterations
  }
  worker_table->Get(data, size);

  printf("----------------------------\n");
  for (int i = 0; i < num_row; ++i){
    printf("rank %d, row %d: ", MV_Rank(), i);
    for (int j = 0; j < num_col; ++j)
      printf("%d ", data[i * num_col + j]);
    printf("\n");
  }

  MV_ShutDown();
}

void TestComm(int argc, char* argv[]) {

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
    else if (strcmp(argv[1], "matrix") == 0) TestMatrix(argc, argv);
    else if (strcmp(argv[1], "nonet") == 0) TestNoNet(argc, argv);
    else if (strcmp(argv[1], "checkpoint") == 0)  TestCheckPoint(argc, argv, false);
    else if (strcmp(argv[1], "restore") == 0) TestCheckPoint(argc, argv, true);
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
