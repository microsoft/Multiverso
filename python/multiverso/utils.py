#!/usr/bin/env python
# coding:utf8

import ctypes
import os

PROJECT_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.path.pardir, os.path.pardir))


class Loader(object):

    LIB = None

    @classmethod
    def load_lib(cls):
        # TODO: write some scripts load .so or .dll
        # TODO: adapt it for windows
        path = os.path.join(PROJECT_PATH,"build", "src", "libmultiverso.so")
        return ctypes.cdll.LoadLibrary(path)

    @classmethod
    def get_lib(cls):
        if not cls.LIB:
            cls.LIB = cls.load_lib()
        return cls.LIB
