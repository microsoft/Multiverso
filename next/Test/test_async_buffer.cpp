#include <gtest/gtest.h>
#include <vector>

#include <cctype>


#include <multiverso/multiverso.h>
#include <multiverso/table/smooth_array_table.h>
#include <multiverso/util/async_buffer.h>


const int array_size = 5;


class test_async_buffer : public ::testing::Test    {
public:
    test_async_buffer(){
    }

    void SetUp() override{
        multiverso::MV_Init();

        worker_array = new multiverso::SmoothArrayWorker<float>(array_size);
        server_array = new multiverso::SmoothArrayServer<float>(array_size);
        p1 = new float[array_size]{0, 0, 0, 0, 0};
        p2 = new float[array_size]{0, 1, 2, 3, 4};

    }

    void TearDown() override{
        delete[]p2;
        delete[]p1;
        // Need to delete array before shutdown?
        //delete server_array;
        //delete worder_array;
        multiverso::MV_ShutDown();
    }

protected:
    multiverso::SmoothArrayWorker<float>* worker_array;
    multiverso::SmoothArrayServer<float>* server_array;
    float* p1;
    float* p2;
};

TEST_F(test_async_buffer, simple_test){
    const auto& worker = this->worker_array;
    auto size = array_size;
    multiverso::ASyncBuffer<float*> async_buffer(p1, p2,
        [worker, size](float * buf) -> void {
        worker->Add(buf, size);
    });
    auto ready_buffer_1 = async_buffer.get_ready_buffer();
    // once you call another get_ready_buffer, the previous ready buffer will be expired. So please keep the buffer content if you need before you calling another get_ready_buffer;
    auto ready_buffer_2 = async_buffer.get_ready_buffer();
    ASSERT_TRUE(true);
}
