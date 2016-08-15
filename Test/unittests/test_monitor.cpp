#include <boost/test/unit_test.hpp>
#include <multiverso/dashboard.h>

namespace multiverso {
namespace test {

BOOST_AUTO_TEST_SUITE(monitor)

BOOST_AUTO_TEST_CASE(monitor) {
  Monitor monitor("unittest_monitor");
  monitor.Begin();
  monitor.End();
  double elapsed_first = monitor.elapse();

  monitor.Begin();
  monitor.End();
  double elapsed_second = monitor.elapse();

  BOOST_CHECK_LE(elapsed_first, elapsed_second);
  BOOST_CHECK_EQUAL(monitor.count(), 2);

  BOOST_CHECK_EQUAL(Dashboard::Watch("unittest_monitor"), 
                    monitor.info_string());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace test
}  // namespace multiverso