#!/usr/bin/env python
# coding:utf8

import ctypes
import os

PACKAGE_PATH = os.path.abspath(os.path.dirname(__file__))


class Loader(object):

    LIB = None

    @classmethod
    def load_lib(cls):
        # TODO: write some scripts load .so or .dll
        # TODO: adapt it for windows
        path = os.path.join(PACKAGE_PATH, "libmultiverso.so")
        return ctypes.cdll.LoadLibrary(path)

    @classmethod
    def get_lib(cls):
        if not cls.LIB:
            cls.LIB = cls.load_lib()
            cls.LIB.MV_NumWorkers.restype = ctypes.c_int
        return cls.LIB
