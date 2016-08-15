#include <multiverso/blob.h>
#include <boost/test/unit_test.hpp>

#define BOOST_TEST_MODULE blob test

BOOST_AUTO_TEST_CASE(blob_constructor_test) {
  multiverso::Blob blob;
  BOOST_CHECK(blob.size() == 0);
}