#!/usr/bin/env python
# coding:utf8

import ctypes
from utils import Loader
import numpy as np


mv_lib = Loader.get_lib()


class TableHandler(object):
    '''`TableHandler` is an interface to sync different kinds of values.

    If you are not writing python code based on theano or lasagne, you are
    supposed to sync models (for initialization) and gradients (during
    training) so as to let multiverso help you manage the models in distributed
    environments.
    Otherwise, you'd better use the classes in `multiverso.theano_ext` or
    `multiverso.theano_ext.lasagne_ext`
    '''
    def __init__(self, size):
        raise NotImplementedError("You must implement the __init__ method.")

    def get(self, size):
        raise NotImplementedError("You must implement the get method.")

    def add(self, data):
        raise NotImplementedError("You must implement the add method.")


# types
C_FLOAT_P = ctypes.POINTER(ctypes.c_float)


class ArrayTableHandler(TableHandler):
    '''`ArrayTableHandler` is used to sync array-like (one-dimensional) value.'''
    def __init__(self, size):
        '''Constructor for syncing array-like (one-dimensional) value.

        The `size` should be a int equal to the size of value we want to sync.
        '''
        self._handler = ctypes.c_void_p()
        self._size = size
        mv_lib.MV_NewArrayTable(size, ctypes.byref(self._handler))

    def get(self):
        '''get the latest value from multiverso ArrayTable

        Data type of return value is numpy.ndarray with one-dimensional
        '''
        data = np.zeros((self._size, ), dtype=np.dtype("float32"))
        mv_lib.MV_GetArrayTable(self._handler, data.ctypes.data_as(C_FLOAT_P), self._size)
        return data

    def add(self, data):
        '''add the data to the multiverso ArrayTable

        Data type of `data` is numpy.ndarray with one-dimensional
        '''
        if not isinstance(data, np.ndarray):
            data = np.array(data)
        assert(data.size == self._size)
        data = data.astype(np.float32)
        mv_lib.MV_AddArrayTable(self._handler, data.ctypes.data_as(C_FLOAT_P), self._size)


class MatrixTableHandler(TableHandler):
    def __init__(self, num_row, num_col):
        '''Constructor for syncing matrix-like (two-dimensional) value.

        The `num_row` should be the number of rows and the `num_col` should be
        the number of columns.
        '''
        self._handler = ctypes.c_void_p()
        self._num_row = num_row
        self._num_col = num_col
        self._size = num_col * num_row
        mv_lib.MV_NewMatrixTable(num_row, num_col, ctypes.byref(self._handler))

    def get(self, row_ids=None):
        '''get the latest value from multiverso MatrixTable

        If row_ids is None, we will return all rows as numpy.narray , e.g.
        array([[1, 3], [3, 4]]).
        Otherwise we will return the data according to the row_ids(e.g. you can
        pass [1] to row_ids to get only the first row, it will return a
        two-dimensional numpy.ndarray with one row)

        Data type of return value is numpy.ndarray with two-dimensional
        '''
        if row_ids is None:
            data = np.zeros((self._num_row, self._num_col), dtype=np.dtype("float32"))
            mv_lib.MV_GetMatrixTableAll(self._handler, data.ctypes.data_as(C_FLOAT_P), self._size)
            return data
        else:
            row_ids_n = len(row_ids)
            int_array_type = ctypes.c_int * row_ids_n
            data = np.zeros((row_ids_n, self._num_col), dtype=np.dtype("float32"))
            mv_lib.MV_GetMatrixTableByRows(self._handler, data.ctypes.data_as(C_FLOAT_P),
                                           row_ids_n * self._num_col,
                                           int_array_type(*row_ids), row_ids_n)
            return data

    def add(self, data=None, row_ids=None):
        '''add the data to the multiverso MatrixTable

        If row_ids is None, we will add all data, and the data
        should be a list, e.g. [1, 2, 3, ...]

        Otherwise we will add the data according to the row_ids

        Data type of `data` is numpy.ndarray with two-dimensional
        '''
        assert(data is not None)
        if not isinstance(data, np.ndarray):
            data = np.array(data)
        data = data.astype(np.float32)

        if row_ids is None:
            assert(data.size == self._size)
            mv_lib.MV_AddMatrixTableAll(self._handler, data.ctypes.data_as(C_FLOAT_P), self._size)
        else:
            row_ids_n = len(row_ids)
            assert(data.size == row_ids_n * self._num_col)
            int_array_type = ctypes.c_int * row_ids_n
            mv_lib.MV_AddMatrixTableByRows(self._handler, data.ctypes.data_as(C_FLOAT_P),
                                           row_ids_n * self._num_col,
                                           int_array_type(*row_ids), row_ids_n)
