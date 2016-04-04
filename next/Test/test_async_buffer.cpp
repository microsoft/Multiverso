#include <gtest/gtest.h>
#include <multiverso/multiverso.h>
#include <multiverso/table/smooth_array_table.h>
#include <multiverso/util/async_buffer.h>

const int array_size = 5;

class test_async_buffer : public ::testing::Test {
 public:
    test_async_buffer() {
    }

    void SetUp() override {
        multiverso::MV_Init();
        worker_array = new multiverso::SmoothArrayWorker<float>(array_size);
        server_array = new multiverso::SmoothArrayServer<float>(array_size);
        p0 = new float[array_size]{};
        p1 = new float[array_size]{};
        multiverso::MV_Barrier();
    }

    void TearDown() override {
        delete[]p1;
        delete[]p0;
        // TODO(junli): confirm: Shall we need to delete array before shutdown?
        // delete server_array;
        // delete worder_array;
        multiverso::MV_Barrier();
        multiverso::MV_ShutDown();
    }

    static bool elementwise_equal(float * array, size_t size, float target) {
        for (auto i = 0; i < size; ++i) {
            if (abs(array[i] - target) > 1e-6) {
                return false;
            }
        }

        return true;
    }

    static void elementwise_set(float * array, size_t size, float target) {
        for (auto i = 0; i < size; ++i) array[i] = target;
    }

 protected:
    multiverso::SmoothArrayWorker<float>* worker_array;
    multiverso::SmoothArrayServer<float>* server_array;
    float* p0;
    float* p1;
};

TEST_F(test_async_buffer, get_async_buffer_from_server) {
    const auto& worker = this->worker_array;
    auto size = array_size;

    // creates async_buffer
    multiverso::ASyncBuffer<float*> async_buffer(p0, p1,
        [worker, size](float * buf) -> void {
        worker->Get(buf, size);
    });

    // get ready buffer and submit an update A
    auto ready_buffer = async_buffer.get_ready_buffer();
    ASSERT_EQ(p0, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 0));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));    // calculate updates A
    elementwise_set(ready_buffer, size, 1);
    worker_array->Add(ready_buffer, size, 0);                       // submit update A

    // get ready buffer and submit an update B, the buffer content should NOT include changes from update A
    ready_buffer = async_buffer.get_ready_buffer();
    ASSERT_EQ(p1, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 0));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));    // calculate update B
    elementwise_set(ready_buffer, size, 1);
    worker_array->Add(ready_buffer, size, 0);                       // submit update B

    // get ready buffer, the buffer content should include changes from update A
    ready_buffer = async_buffer.get_ready_buffer();
    ASSERT_EQ(p0, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 1));

    // get ready buffer, the buffer content should include changes from update A and update B
    ready_buffer = async_buffer.get_ready_buffer();
    ASSERT_EQ(p1, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 2));

    // stop prefetch and release related resource
    async_buffer.stop_prefetch();

    // restart prefetch after stopping
    ready_buffer = async_buffer.get_ready_buffer();
    ASSERT_EQ(p0, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 2));

    // stop prefetch and release related resource
    async_buffer.stop_prefetch();
}
