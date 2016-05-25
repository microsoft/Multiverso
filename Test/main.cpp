#include <iostream>
#include <thread>
#include <random>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <memory>

#include <mpi.h>

#include <multiverso/multiverso.h>
#include <multiverso/net.h>
#include <multiverso/util/log.h>
#include <multiverso/util/net_util.h>
#include <multiverso/util/configure.h>
#include <multiverso/util/timer.h>
#include <multiverso/dashboard.h>

#include <multiverso/table/array_table.h>
#include <multiverso/table/kv_table.h>
#include <multiverso/table/matrix_table.h>             
#include <multiverso/table/sparse_matrix_table.h>
#include <multiverso/updater/updater.h>
#include <multiverso/table_factory.h>

#include <gtest/gtest.h>

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

  multiverso::SetCMDFlag("sync", true);
  MV_Init(&argc, argv);

  size_t array_size = 5;

  ArrayWorker<int>* shared_array = MV_CreateTable(ArrayTableOption<int>(array_size));

  MV_Barrier();
  Log::Info("Create tables OK. Rank = %d, worker_id = %d\n", MV_Rank(), MV_WorkerId());

  std::vector<int> delta(array_size);
  for (int i = 0; i < array_size; ++i)
    delta[i] = static_cast<int>(i);
  int* data = new int[array_size];

  int iter = 1000000000;

  for (int i = 0; i < iter; ++i) {
    shared_array->Add(delta.data(), array_size);
    shared_array->Get(data, array_size);
    for (int k = 0; k < array_size; ++k) {
      if (data[k] != delta[k] * (i + 1) * MV_NumWorkers()) {
        std::cout << "i + 1 = " << i + 1 << " k = " << k << std::endl;
        for (int j = 0; j < array_size; ++j) {
          std::cout << data[j] << " ";
        }
        exit(1);
      }
    }
    if (i % 1000 == 0) { printf("iter = %d\n", i); fflush(stdout); }
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
      while (net->Send(msg) == 0);
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
    while (net->Send(msg) == 0);
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

//void TestMatrix(int argc, char* argv[]){
//  Log::Info("Test Matrix\n");
//
//  Log::ResetLogLevel(LogLevel::Info);
//  multiverso::SetCMDFlag("sync", true);
//  MV_Init(&argc, argv);
//
//  int num_row = 8, num_col = 3592;
//  int size = num_row * num_col;
//  // MatrixWorkerTable<int>* worker_table = new MatrixWorkerTable<int>(num_row, num_col);
//  // MatrixServerTable<int>* server_table = new MatrixServerTable<int>(num_row, num_col);
//  MatrixTableOption<int> option;
//  option.num_row = num_row;
//  option.num_col = num_col;
//  MatrixWorkerTable<int>* worker_table = multiverso::MV_CreateTable(option);
//  std::thread* m_prefetchThread = nullptr;
//  MV_Barrier();
//  int count = 0;
//  while (true)
//  {
//    count++;
//    std::vector<int> v = { 0, 1, 3, 7 };
//
//    // test data
//    std::vector<int> delta(size);
//    std::vector<int> data(size, 0);
//    for (auto i = 0; i < size; ++i)
//      delta[i] = (int)i;
//
//    worker_table->Add(delta.data(), size);
//    worker_table->Get(data.data(), size);
//    if (count % 1000 == 0)
//    { 
//      printf("Dense Add/Get, #test: %d.\n", count);
//      fflush(stdout);
//    }
//
//    std::vector<int*> data_rows = { &data[0], &data[num_col], &data[3 * num_col], &data[7 * num_col] };
//    std::vector<int*> delta_rows = { &delta[0], &delta[num_col], &delta[3 * num_col], &delta[7 * num_col] };
//    worker_table->Add(v, delta_rows, num_col);
//    worker_table->Get(v, data_rows, num_col);
//    //MV_Barrier();
//    //worker_table->Get(v, data_rows, num_col);
//
//    if (count % 1000 == 0)
//    {
//      printf("Sparse Add/Get, #test: %d.\n", count);
//      fflush(stdout);
//    }
//    for (auto i = 0; i < num_row; ++i) {
//      for (auto j = 0; j < num_col; ++j) {
//        int expected = (int)(i * num_col + j) * count * MV_NumWorkers();
//        if (i == 0 || i == 1 || i == 3 || i == 7) {
//          expected += (int)(i * num_col + j) * count * MV_NumWorkers();
//        }
//        int actual = data[i* num_col + j];
//        ASSERT_EQ(expected, actual) << "Should be equal after adding, row: " 
//        << i << ", col:" << j << ", expected: " << expected << ", actual: " << actual;
//      }
//    }
//
//  }
//  delete worker_table;
//  MV_ShutDown();
////}


void TestMatrix(int argc, char* argv[]){
  // Log::ResetLogLevel(LogLevel::Debug);
  multiverso::SetCMDFlag("sync", true);
  MV_Init(&argc, argv);

  int num_row = 8, num_col = 3592;
  int num_tables = 5;
  std::vector<int> num_table_size;
  std::vector<MatrixTableOption<int>* > table_options;
  std::vector<MatrixWorkerTable<int>* > worker_tables;

  for (auto i = 0; i < num_tables - 1 ; i++)
  {
    table_options.push_back(new MatrixTableOption<int>());
    table_options[i]->num_col = num_col;
    table_options[i]->num_row = num_row + i;
    num_table_size.push_back(num_col * (num_row + i));
    worker_tables.push_back(multiverso::MV_CreateTable(*table_options[i]));
  }

  table_options.push_back(new MatrixTableOption<int>());
  table_options[4]->num_col = num_col;
  table_options[4]->num_row = 1;
  num_table_size.push_back(num_col * (1));
  worker_tables.push_back(multiverso::MV_CreateTable(*table_options[4]));

  std::thread* m_prefetchThread = nullptr;
  MV_Barrier();
  int count = 0;
  while (true)
  {
    count++;
    std::vector<int> v = { 0, 1, 3, 7 };

    // test data
    std::vector<std::vector<int>> delta(num_tables);
    std::vector<std::vector<int>> data(num_tables);
    for (auto j =0; j < num_tables; j++)
    {
      delta[j].resize(num_table_size[j]);
      data[j].resize(num_table_size[j], 0);
      for (auto i = 0; i < num_table_size[j]; ++i)
        delta[j][i] = (int)i;
    }

    for (auto j = 0; j < num_tables; j++)
    { 
      worker_tables[j]->Add(delta[j].data(), num_table_size[j]);
      worker_tables[j]->Get(data[j].data(), num_table_size[j]);
    }
    if (count % 1000 == 0)
    { 
      printf("Dense Add/Get, #test: %d.\n", count);
      fflush(stdout);
    }

    std::vector<int*> data_rows = { &data[0][0], &data[0][num_col], &data[0][3 * num_col], &data[0][7 * num_col] };
    std::vector<int*> delta_rows = { &delta[0][0], &delta[0][num_col], &delta[0][3 * num_col], &delta[0][7 * num_col] };
    for (auto j = 0; j < num_tables - 1; j++)
    { 
      worker_tables[j]->Add(v, delta_rows, num_col);
      worker_tables[j]->Get(v, data_rows, num_col);
    }
    //MV_Barrier();
    //worker_table->Get(v, data_rows, num_col);

    if (count % 1000 == 0)
    {
      printf("Sparse Add/Get, #test: %d.\n", count);
      fflush(stdout);
    }
    for (auto i = 0; i < num_row; ++i) {
      for (auto j = 0; j < num_col; ++j) {
        int expected = (int)(i * num_col + j) * count * MV_NumWorkers();
        if (i == 0 || i == 1 || i == 3 || i == 7) {
          expected += (int)(i * num_col + j) * count * MV_NumWorkers();
        }
        int actual = data[0][i* num_col + j];
        ASSERT_EQ(expected, actual) << "Should be equal after adding, row: " 
        << i << ", col:" << j << ", expected: " << expected << ", actual: " << actual;
      }
    }

  }
  worker_tables.clear();
  MV_ShutDown();
}
void TestCheckPoint(int argc, char* argv[], bool restore){
  Log::Info("Test CheckPoint\n");

  MV_Init(&argc, argv);

  int num_row = 11, num_col = 10;
  int size = num_row * num_col;

  MatrixWorkerTable<int>* worker_table = new MatrixWorkerTable<int>(num_row, num_col);
  MatrixServerTable<int>* server_table = new MatrixServerTable<int>(num_row, num_col);

  //if restore = true, will restore server data and return the next iter number of last dump file
  //else do nothing and return 0
  // int begin_iter = MV_LoadTable("serverTable_");
  MV_Barrier();//won't dump data without parameters

  std::vector<int> delta(size);
  for (int i = 0; i < size; ++i)
    delta[i] = i;
  int * data = new int[size];

  // Log::Debug("rank %d start from iteration %d\n", MV_Rank(), begin_iter);

  for (int i = 0 /*begin_iter*/; i < 50; ++i){
    worker_table->Add(delta.data(), size);
    MV_Barrier(); //dump table data with iteration i each k iterations
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

void TestAllreduce(int argc, char* argv[]) {
  multiverso::SetCMDFlag("ma", true);
  MV_Init(&argc, argv);
  int a = 1;
  MV_Aggregate(&a, 1);
  std::cout << "a = " << a << std::endl;
  MV_ShutDown();
}

template<typename WT, typename ST>
void TestmatrixPerformance(int argc, char* argv[],
  std::function<std::shared_ptr<WT>(int num_row, int num_col)>CreateWorkerTable,
  std::function<std::shared_ptr<ST>(int num_row, int num_col)>CreateServerTable,
  std::function<void(const std::shared_ptr<WT>& worker_table, const std::vector<int>& row_ids, const std::vector<float*>& data_vec, size_t size, const AddOption* option, int worker_id)> Add,
  std::function<void(const std::shared_ptr<WT>& worker_table, float* data, size_t size, int worker_id)> Get) {

  Log::ResetLogLevel(LogLevel::Info);
  Log::Info("Test Matrix\n");
  Timer timmer;

  //multiverso::SetCMDFlag("sync", true);
  MV_Init(&argc, argv);
  int per = 0;
  int num_row = 1000000, num_col = 50;
  if (argc == 3){
    num_row = atoi(argv[2]);
  }

  int size = num_row * num_col;
  int worker_id = MV_Rank();
  int worker_num = MV_Size();

  // test data
  float* data = new float[size];
  float* delta = new float[size];
  int* keys = new int[num_row];
  for (auto row = 0; row < num_row; ++row) {
    for (auto col = 0; col < num_col; ++col) {
      delta[row * num_col + col] = row * num_col + col;
    }
  }

  AddOption option;
  option.set_worker_id(worker_id);
  //std::mt19937_64 eng{ std::random_device{}() };
  //std::vector<int> unique_index;
  //for (int i = 0; i < num_row; i++){
  //  unique_index.push_back(i);
  //}
  
  for (auto percent = 0; percent < 10; ++percent)
    for (auto turn = 0; turn < 10; ++turn)
    {
      //std::shuffle(unique_index.begin(), unique_index.end(), eng);
      if (worker_id == 0) {
        std::cout << "\nTesting: Get All Rows => Add "
          << percent + 1 << "0% Rows to Server => Get All Rows" << std::endl;
      }


      auto worker_table = CreateWorkerTable(num_row, num_col);
      auto server_table = CreateServerTable(num_row, num_col);
      MV_Barrier();

      timmer.Start();
      Get(worker_table, data, size, worker_id);
      std::cout << " " << 1.0 * timmer.elapse() / 1000 << "s:\t" << "get all rows first time, worker id: " << worker_id << std::endl;
      MV_Barrier();

      std::vector<int> row_ids;
      std::vector<float*> data_vec;
      for (auto i = 0; i < num_row; ++i) {
        if (i % 10 <= percent && i % worker_num == worker_id) {
          row_ids.push_back(i);
          data_vec.push_back(delta + i * num_col);
        }
      }
      //for (auto i = 0; i < (percent + 1) * num_row / 10; i++)
      //{
      //  row_ids.push_back(unique_index[i]);
      //  data_vec.push_back(delta + unique_index[i] * num_col);
      //}

      if (worker_id == 0) {
        std::cout << "adding " << percent + 1 << " /10 rows to matrix server" << std::endl;
      }

      if (row_ids.size() > 0) {
        Add(worker_table, row_ids, data_vec, num_col, &option, worker_id);
      }
      Get(worker_table, data, size, -1);
      MV_Barrier();

      timmer.Start();
      Get(worker_table, data, size, worker_id);
      std::cout << " " << 1.0 * timmer.elapse() / 1000 << "s:\t" << "get all rows after adding to rows, worker id: " << worker_id << std::endl;

      for (auto i = 0; i < num_row; ++i) {
        auto row_start = data + i * num_col;
        for (auto col = 0; col < num_col; ++col) {
          float expected = (float) i * num_col + col;
          float actual = *(row_start + col);
          if (i % 10 <= percent) {
            ASSERT_FLOAT_EQ(expected, actual) << "Should be updated after adding, worker_id:"
              << worker_id << ",row: " << i << ",col:" << col << ",expected: " << expected << ",actual: " << actual;
          }
          else {
            ASSERT_FLOAT_EQ(0, *(row_start + col)) << "Should be 0 for non update row values, worker_id:"
              << worker_id << ",row: " << i << ",col:" << col << ",expected: " << expected << ",actual: " << actual;
          }
        }
      }

      MV_Barrier();
    }

  MV_Barrier();
  Log::ResetLogLevel(LogLevel::Info);
  Dashboard::Display();
  Log::ResetLogLevel(LogLevel::Error);
  MV_ShutDown();
}

void TestSparsePerf(int argc, char* argv[]) {
  TestmatrixPerformance<SparseMatrixWorkerTable<float>, SparseMatrixServerTable<float>>(argc,
    argv,
    [](int num_row, int num_col) {
    return std::shared_ptr<SparseMatrixWorkerTable<float>>(
      new SparseMatrixWorkerTable<float>(num_row, num_col));
  },
    [](int num_row, int num_col) {
    return std::shared_ptr<SparseMatrixServerTable<float>>(
      new SparseMatrixServerTable<float>(num_row, num_col, false));
  },
    [](const std::shared_ptr<SparseMatrixWorkerTable<float>>& worker_table, const std::vector<int>& row_ids, const std::vector<float*>& data_vec, size_t size, const AddOption* option, const int worker_id) {
    worker_table->Add(row_ids, data_vec, size, option);
  },

    [](const std::shared_ptr<SparseMatrixWorkerTable<float>>& worker_table, float* data, size_t size, int worker_id) {
    GetOption get_option;
    get_option.set_worker_id(worker_id);
    worker_table->Get(data, size, &get_option);
  });
}


void TestDensePerf(int argc, char* argv[]) {
  TestmatrixPerformance<MatrixWorkerTable<float>, MatrixServerTable<float>>(argc,
    argv,
    [](int num_row, int num_col) {
    return std::shared_ptr<MatrixWorkerTable<float>>(
      new MatrixWorkerTable<float>(num_row, num_col));
  },
    [](int num_row, int num_col) {
    return std::shared_ptr<MatrixServerTable<float>>(
      new MatrixServerTable<float>(num_row, num_col));
  },
    [](const std::shared_ptr<MatrixWorkerTable<float>>& worker_table, const std::vector<int>& row_ids, const std::vector<float*>& data_vec, size_t size, const AddOption* option, const int worker_id) {
    worker_table->Add(row_ids, data_vec, size, option);
  },

    [](const std::shared_ptr<MatrixWorkerTable<float>>& worker_table, float* data, size_t size, int worker_id) {
    worker_table->Get(data, size);
  });
}

int main(int argc, char* argv[]) {
  Log::ResetLogLevel(LogLevel::Info);

  if (argc == 1){
    multiverso::MV_Init();
    ::testing::InitGoogleTest(&argc, argv);
    auto res = RUN_ALL_TESTS();
    multiverso::MV_ShutDown();
    return res;
  } else {
    if (strcmp(argv[1], "kv") == 0) TestKV(argc, argv);
    else if (strcmp(argv[1], "array") == 0) TestArray(argc, argv);
    else if (strcmp(argv[1], "net") == 0) TestNet(argc, argv);
    else if (strcmp(argv[1], "ip") == 0) TestIP();
    else if (strcmp(argv[1], "matrix") == 0) TestMatrix(argc, argv);
    else if (strcmp(argv[1], "checkpoint") == 0)  TestCheckPoint(argc, argv, false);
    else if (strcmp(argv[1], "restore") == 0) TestCheckPoint(argc, argv, true);
    else if (strcmp(argv[1], "allreduce") == 0) TestAllreduce(argc, argv);
    else if (strcmp(argv[1], "TestSparsePerf") == 0) TestSparsePerf(argc, argv);
    else if (strcmp(argv[1], "TestDensePerf") == 0) TestDensePerf(argc, argv);
    else CHECK(false);
  }
  return 0;
}
