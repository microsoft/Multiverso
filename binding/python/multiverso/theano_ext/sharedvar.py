#!/usr/bin/env python
# coding:utf8

from theano.tensor.basic import TensorType, _tensor_py_operators
from theano.compile import SharedVariable
from theano.compile.sharedvalue import shared
from theano.gof import Variable, utils
import numpy
import multiverso as mv


class MVSharedVariable(object):
    '''
    MVSharedVariable will act like a SharedVariable wrapper.
    '''
    def __init__(self, svobj):
        assert(isinstance(svobj, SharedVariable))
        self._svobj = svobj
        self._mv_array = mv.ArrayTableHandler(self._svobj.get_value().size)

        self._mv_array.add(self._svobj.get_value().reshape((-1,)))

        # I restore a copy of value. It will be used for calculate the update
        # for multiverso when calling mv_sync
        self._last_mv_data = self._svobj.get_value(borrow=False)

    def mv_sync(self):
        '''
        This will add the delta of SharedVariable to parameter server and then
        get the latest value in multiverso
        '''
        # because multiverso always use add method to sync value, the delta
        # will be the difference of the current value of last synced value
        self._mv_array.add(self._svobj.get_value() - self._last_mv_data)

        self._svobj.set_value(self._mv_array.get().reshape(self._svobj.get_value().shape))
        self._last_mv_data = self._svobj.get_value(borrow=False)

    def __getstate__(self):
        '''
        This is for cPickle to store state.
        '''
        odict = self.__dict__.copy()  # copy the dict since we change it
        del odict['_mv_array']  # remove mv_array, because we can't pickle it
        return odict

    def __getattribute__(self, attr):
        if attr in ['_svobj', '_mv_array', '_last_mv_data']:
            # If get the attribute of self, use parent __getattribute__ to get
            # attribute from the object, otherwise it will fall into infinite
            # loop
            return object.__getattribute__(self, attr)
        elif attr in ['mv_sync', "__getstate__"]:
            # If call method of MVSharedVariable, then call the method directly
            # and bound the method to self object
            return getattr(MVSharedVariable, attr).__get__(self)
        else:
            # Otherwise I will get attribute from the wrapped object
            return getattr(self._svobj, attr)


def mv_shared(*args, **kwargs):
    var = shared(*args, **kwargs)
    mv_shared.shared_vars.append(MVSharedVariable(var))
    return var


mv_shared.shared_vars = []  # all shared_vars in multiverso will be recorded here


def sync_all_mv_shared_vars():
    '''
    Sync value with multiverso. It is often used when you are training model,
    and it will add the gradients (delta value) to the server and update the
    latest value from the server.
    '''
    for sv in mv_shared.shared_vars:
        sv.mv_sync()
