#!/usr/bin/env python
# coding:utf8

from keras.callbacks import Callback
from param_manager import KerasParamManager


class MVCallback(Callback):
    '''
    Please use MVCallback as a callback of keras model.fit function
    For e.g.
    ```
    model.fit(X_train, Y_train,
              batch_size=batch_size,
              nb_epoch=nb_epoch,
              validation_data=(X_test, Y_test),
              shuffle=True,
              callbacks=[mvcallback(model)])
    ```
    '''
    def __init__(self, model):
        super(MVCallback, self).__init__()
        self.kpm = KerasParamManager(model)

    def on_batch_end(self, batch, logs={}):
        '''sync all parameters at the end of every batch'''
        self.kpm.sync_all_param()
