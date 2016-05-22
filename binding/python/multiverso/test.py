#!/usr/bin/env python
# coding:utf8
import api as mv


def TestArray():
    size = 100000
    mv.init()
    tbh = mv.ArrayTableHandler(size)
    mv.barrier()

    for i in xrange(1000):
        print tbh.get()[:10]
        tbh.add(range(size))
        tbh.add(range(size))
        mv.barrier()
    mv.shutdown()


def TestMatrix():
    mv.init()
    num_row = 11
    num_col = 10
    size = num_col * num_row
    tbh = mv.MatrixTableHandler(num_row, num_col)
    mv.barrier()
    for count in xrange(1, 20):
        row_ids = [0, 1, 5, 10]
        tbh.add(range(size))
        tbh.add([range(rid * num_col, (1 + rid) * num_col) for rid in row_ids], row_ids)
        mv.barrier()
        data = tbh.get()
        mv.barrier()
        for i, row in enumerate(data):
            for j, actual in enumerate(row):
                expected = (i * num_col + j) * count * mv.workers_num()
                if i in row_ids:
                    expected += (i * num_col + j) * count * mv.workers_num()
                assert(expected == actual)
    mv.shutdown()


if __name__ == "__main__":
    # TODO: add arguments to choose which test to run
    # TestArray()
    TestMatrix()
