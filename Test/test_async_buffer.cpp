#include <gtest/gtest.h>
#include <multiverso/multiverso.h>
#include <multiverso/table/array_table.h>
#include <multiverso/util/async_buffer.h>

const int array_size = 5;

class test_async_buffer : public ::testing::Test {
 public:
    test_async_buffer() {
    }

    void SetUp() override {
        worker_array = new multiverso::ArrayWorker<float>(array_size);
        server_array = new multiverso::ArrayServer<float>(array_size);
        p0 = new float[array_size]{};
        p1 = new float[array_size]{};
        multiverso::MV_Barrier();
    }

    void TearDown() override {
        delete[]p1;
        delete[]p0;
        delete server_array;
        delete worker_array;
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
    multiverso::ArrayWorker<float>* worker_array;
    multiverso::ArrayServer<float>* server_array;
    float* p0;
    float* p1;
};

TEST_F(test_async_buffer, get_async_buffer_from_server) {
    const auto& worker = this->worker_array;
    auto size = array_size;

    // creates async_buffer
    multiverso::ASyncBuffer<float> async_buffer(p0, p1,
        [worker, size](float * buf) -> void {
        worker->Get(buf, size);
    });

    // get ready buffer and submit an update A
    auto ready_buffer = async_buffer.Get();
    ASSERT_EQ(p0, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 0));
    // calculate updates A
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    elementwise_set(ready_buffer, size, 1);
    // submit update A
    worker_array->Add(ready_buffer, size, 0);

    // get ready buffer and submit an update B,
    //  the buffer content should NOT include changes from update A
    ready_buffer = async_buffer.Get();
    ASSERT_EQ(p1, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 0));
    // calculate update B
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    elementwise_set(ready_buffer, size, 1);
    // submit update B
    worker_array->Add(ready_buffer, size, 0);

    // get ready buffer, the buffer content should
    //  include changes from update A
    ready_buffer = async_buffer.Get();
    ASSERT_EQ(p0, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 1));

    // get ready buffer, the buffer content should
    //  include changes from update A and update B
    ready_buffer = async_buffer.Get();
    ASSERT_EQ(p1, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 2));

    // stop prefetch and release related resource
    async_buffer.Join();

    // restart prefetch after stopping
    ready_buffer = async_buffer.Get();
    ASSERT_EQ(p0, ready_buffer);
    ASSERT_TRUE(elementwise_equal(ready_buffer, size, 2));

    // stop prefetch and release related resource
    async_buffer.Join();
}
