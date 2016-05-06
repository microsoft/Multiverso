#!/usr/bin/env python
# coding:utf8

from ctypes import *
from utils import Loader


LIB = Loader.get_lib()


def MV_Init(args=[]):
    n = len(args)
    args_type = c_char_p * n
    LIB.MV_Init(n, args_type(*[c_char_p(arg) for arg in args]))


def MV_ShutDown():
    LIB.MV_ShutDown()


def MV_Barrier():
    LIB.MV_Barrier()


class TableHandle(object):
    def __init__(self, size):
        self._handler = c_void_p()
        LIB.MV_NewTable(size, byref(self._handler))

    def get(self, size):
        c_data = (c_float * size)()
        LIB.MV_Get(self._handler, c_data, size)
        return [d for d in c_data]

    def add(self, data):
        size = len(data)
        LIB.MV_Add(self._handler, (c_float * size)(*data), size)
