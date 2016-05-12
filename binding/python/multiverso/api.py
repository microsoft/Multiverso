#!/usr/bin/env python
# coding:utf8

from ctypes import *
from utils import Loader
import numpy as np


LIB = Loader.get_lib()


def MV_Init(args=[]):
    n = len(args)
    args_type = c_char_p * n
    LIB.MV_Init(n, args_type(*[c_char_p(arg) for arg in args]))


def MV_ShutDown():
    LIB.MV_ShutDown()


def MV_Barrier():
    LIB.MV_Barrier()


def MV_NumWorkers():
    return LIB.MV_NumWorkers()


def MV_WorkerId():
    return LIB.MV_WorkerId()


def MV_ServerId():
    return MV_ServerId()


class TableHandler(object):
    def __init__(self, size):
        raise NotImplementedError("You must implement the __init__ method.")

    def get(self, size):
        raise NotImplementedError("You must implement the get method.")

    def add(self, data):
        raise NotImplementedError("You must implement the add method.")


class ArrayTableHandler(TableHandler):
    def __init__(self, size):
        self._handler = c_void_p()
        self._size = size
        LIB.MV_NewArrayTable(size, byref(self._handler))

    def get(self):
        c_data = (c_float * self._size)()
        LIB.MV_GetArrayTable(self._handler, c_data, self._size)
        return [d for d in c_data]

    def get_array(self):
        return np.array(self.get())

    def add(self, data):
        LIB.MV_AddArrayTable(self._handler, (c_float * self._size)(*data), self._size)

    def add_array(self, data):
        self.add(data.reshape(-1))


class MatrixTableHandler(TableHandler):
    def __init__(self, num_row, num_col):
        self._handler = c_void_p()
        self._num_row = num_row
        self._num_col = num_col
        self._size = num_col * num_row
        LIB.MV_NewMatrixTable(num_row, num_col, byref(self._handler))

    def _construct_matrix(self, c_data):
        res = []
        row = []
        for d in c_data:
            row.append(d)
            if len(row) >= self._num_col:
                res.append(row)
                row = []
        return res

    def get(self, row_ids=None):
        '''
            If row_ids is None, we will return all data in a list , e.g. [1, 2, 3, ...].
            Otherwise we will return the data according to the row_ids
        '''
        if row_ids is None:
            float_array_type = c_float * (self._num_row * self._num_col)
            c_data = float_array_type()
            LIB.MV_GetMatrixTableAll(self._handler, c_data, self._size)
            return self._construct_matrix(c_data)
        else:
            row_ids_n = len(row_ids)
            int_array_type = c_int * row_ids_n
            float_array_array_type = c_float * self._num_col * row_ids_n
            float_pointer_array_type = POINTER(c_float) * row_ids_n

            array_data = float_array_array_type()
            c_data = float_pointer_array_type(*[row for row in array_data])
            LIB.MV_GetMatrixTableByRows(self._handler, int_array_type(*row_ids),
                row_ids_n, self._num_col, c_data)
            return [[d for d in row] for row in array_data]

    def get_array(self):
        '''
            return all data as array with demensions sizes setted
        '''
        return np.array(self.get()).reshape((self._num_row, self._num_col))

    def add(self, data=None, row_ids=None):
        '''
            If row_ids is None, we will add all data, and the data
            should be a list, e.g. [1, 2, 3, ...]

            Otherwise we will add the data according to the row_ids
        '''
        if row_ids is None:
            float_array_type = c_float * (self._num_row * self._num_col)
            c_data = float_array_type(*data)
            LIB.MV_AddMatrixTableAll(self._handler, c_data, self._size)
        else:
            if data is None:
                return None
            row_ids_n = len(row_ids)
            int_array_type = c_int * row_ids_n
            float_array_type = c_float * self._num_col
            float_pointer_array_type = POINTER(c_float) * row_ids_n

            c_data = float_pointer_array_type(*[float_array_type(*row) for row in data])
            LIB.MV_AddMatrixTableByRows(self._handler, int_array_type(*row_ids),
                row_ids_n, self._num_col, c_data)

    def add_array(self, arr):
        self.add(data=arr.reshape(-1))
