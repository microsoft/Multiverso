#!/usr/bin/env python
# coding:utf8
from api import MV_Init, MV_Barrier, MV_ShutDown, ArrayTableHandler, MatrixTableHandler, MV_NumWorkers


def TestArray():
    size = 100000
    MV_Init()
    tbh = ArrayTableHandler(size)
    MV_Barrier()

    for i in xrange(1000):
        print tbh.get(size)[:10]
        tbh.add(range(size))
        tbh.add(range(size))
    MV_ShutDown()


def TestMatrix():
    MV_Init()
    num_row = 11
    num_col = 10
    size = num_col * num_row
    tbh = MatrixTableHandler(num_row, num_col)
    MV_Barrier()
    for count in xrange(1, 20):
        row_ids = [0, 1, 5, 10]
        tbh.add(range(size))
        tbh.add([range(rid * num_col, (1 + rid) * num_col) for rid in row_ids], row_ids)
        data = tbh.get()
        for i, row in enumerate(data):
            for j, actual in enumerate(row):
                expected = (i * num_col + j) * count * MV_NumWorkers()
                if i in row_ids:
                    expected += (i * num_col + j) * count * MV_NumWorkers()
                assert(expected == actual)
        MV_Barrier()
    MV_ShutDown()


if __name__ == "__main__":
    # TODO: add arguments to choose which test to run
    # TestArray()
    TestMatrix()
