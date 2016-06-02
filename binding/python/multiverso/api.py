#!/usr/bin/env python
# coding:utf8

import ctypes
from utils import Loader
import numpy as np


mv_lib = Loader.get_lib()


def init(args=[]):
    n = len(args)
    args_type = ctypes.c_char_p * n
    mv_lib.MV_Init(ctypes.pointer(ctypes.c_int(n)), args_type(*[ctypes.c_char_p(arg) for arg in args]))


def shutdown():
    mv_lib.MV_ShutDown()


def barrier():
    mv_lib.MV_Barrier()


def workers_num():
    return mv_lib.MV_NumWorkers()


def worker_id():
    return mv_lib.MV_WorkerId()


def server_id():
    return mv_lib.MV_ServerId()
