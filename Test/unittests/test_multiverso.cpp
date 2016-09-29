#ifndef _WIN32
// Use dynamic library on linux
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE multiverso
#include <boost/test/unit_test.hpp>
