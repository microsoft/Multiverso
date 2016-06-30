#!/usr/bin/env python
# coding:utf8

import lasagne
import numpy as np
import multiverso as mv


class MVNetParamManager(object):
    '''
    MVNetParamManager is manager to make managing and synchronizing the
    variables in lasagne more easily
    '''
    def __init__(self, network):
        ''' The constructor of MVNetParamManager

        The constructor will associate the parameter with multiverso array
        table.  The initial value of ArrayTableHandler will be same as the
        parameters of network. If different parameters are used in different
        processes, the average of them will be used as the initial value
        '''
        self.shapes = []
        self.dtypes = []
        self.sizes = []
        self.all_param_list = []
        self.network = network
        for arr in lasagne.layers.get_all_param_values(self.network):
            self.shapes.append(arr.shape)
            # TODO: Now only float32 is supported in multiverso. So I store all
            # the parameters in a float32 array. This place need modification
            # after other types are supported
            assert(np.dtype("float32") == arr.dtype)
            self.dtypes.append(arr.dtype)
            self.sizes.append(arr.size)
            self.all_param_list.extend([i for i in np.nditer(arr)])
        self.all_param_list = np.array(self.all_param_list)

        self.tbh = mv.ArrayTableHandler(len(self.all_param_list), init_value=self.all_param_list)
        mv.barrier()  # add barrier to make sure the initial values have token effect
        self.all_param_list = self.tbh.get()
        self._set_all_param_to_net()

    def _set_all_param_to_net(self):
        n = 0
        params = []
        for i, size in enumerate(self.sizes):
            params.append(self.all_param_list[n:n + size].reshape(self.shapes[i]))
            n += size
        lasagne.layers.set_all_param_values(self.network, params)

    def sync_all_param(self):
        '''sync all parameters with multiverso server

        This function will
        1) calc all the delta of params in the network and add the delta to multiverso server
        2) get the latest value from the multiverso server
        '''
        cur_network_params = np.concatenate([
            arr.reshape(-1) for arr in lasagne.layers.get_all_param_values(self.network)])

        params_delta = cur_network_params - self.all_param_list
        self.tbh.add(params_delta)
        self.all_param_list = self.tbh.get()
        self._set_all_param_to_net()
