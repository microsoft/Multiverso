#!/usr/bin/env python
# coding:utf8

import ctypes
import os
import platform

PACKAGE_PATH = os.path.abspath(os.path.dirname(__file__))
PROJECT_PATH = os.path.join(PACKAGE_PATH, os.path.pardir, os.path.pardir, os.path.pardir)


class Loader(object):

    LIB = None

    @classmethod
    def load_lib(cls):
        if platform.system() == "Windows":
            mv_lib_path = os.path.join(PACKAGE_PATH, "Multiverso.dll")
        else:
            mv_lib_path = os.path.join(PACKAGE_PATH, "libmultiverso.so")
        if not os.path.exists(mv_lib_path):
            msg = "The multiverso dynamic library(" + mv_lib_path + ") can't be "\
                  "found  , please make sure you have run python setup.py install"\
                  "to install the multivers-python package successfully"
            # Make the message colorful
            print "\033[93m" + msg + '\033[0m'
        return ctypes.cdll.LoadLibrary(mv_lib_path)

    @classmethod
    def get_lib(cls):
        if not cls.LIB:
            cls.LIB = cls.load_lib()
            cls.LIB.MV_NumWorkers.restype = ctypes.c_int
        return cls.LIB