#!/usr/bin/env python
# coding:utf8
from api import MV_Init, MV_Barrier, TableHandle


def TestArray():
    size = 100000
    MV_Init()
    tbh = TableHandle(size)
    MV_Barrier()

    for i in xrange(1000):
        print tbh.get(size)[:10]
        tbh.add(range(size))
        tbh.add(range(size))


if __name__ == "__main__":
    TestArray()
