#!/usr/bin/env python
# coding:utf8
import multiverso as mv
import unittest


def setUpModule():
    mv.init()


def tearDownModule():
    mv.shutdown()


class TestMultiversoTables(unittest.TestCase):
    '''
    Use the commands below to run test
    $ nosetests
    '''

    def _test_array(self, size):
        tbh = mv.ArrayTableHandler(size)
        mv.barrier()

        for i in xrange(100):
            tbh.add(range(1, size + 1))
            tbh.add(range(1, size + 1))
            mv.barrier()
            for j, actual in enumerate(tbh.get()):
                self.assertEqual((j + 1) * (i + 1) * 2 * mv.workers_num(), actual)
            mv.barrier()

    def test_small_array(self):
        # TODO : this is not supported by multiverso because of the size
        # limited. Waiting for the solution of this issue
        # https://github.com/Microsoft/multiverso/issues/69

        # self._test_array(1)
        pass

    def test_array(self):
        self._test_array(10000)

    def test_matrix(self):
        num_row = 11
        num_col = 10
        size = num_col * num_row
        workers_num = mv.workers_num()
        tbh = mv.MatrixTableHandler(num_row, num_col)
        mv.barrier()
        for count in xrange(1, 21):
            row_ids = [0, 1, 5, 10]
            tbh.add(range(size))
            tbh.add([range(rid * num_col, (1 + rid) * num_col) for rid in row_ids], row_ids)
            mv.barrier()
            data = tbh.get()
            mv.barrier()
            for i, row in enumerate(data):
                for j, actual in enumerate(row):
                    expected = (i * num_col + j) * count * workers_num
                    if i in row_ids:
                        expected += (i * num_col + j) * count * workers_num
                    self.assertEqual(expected, actual)
            data = tbh.get(row_ids)
            mv.barrier()
            for i, row in enumerate(data):
                for j, actual in enumerate(row):
                    expected = (row_ids[i] * num_col + j) * count * workers_num * 2
                    self.assertEqual(expected, actual)


if __name__ == '__main__':
    unittest.main()
