#!/usr/bin/env python
# coding:utf8

"""
This code is adapted from
https://github.com/benanne/theano-tutorial/blob/master/6_convnet.py

The MIT License (MIT)

Copyright (c) 2015 Sander Dieleman

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

"""

import theano
import theano.tensor as T
import numpy as np
import matplotlib.pyplot as plt
plt.ion()

import load_data

from theano.tensor.nnet.conv import conv2d
from theano.tensor.signal.downsample import max_pool_2d
import multiverso as mv
from multiverso.theano_ext.sharedvar import mv_shared, sync_all_mv_shared_vars


x_train, t_train, x_test, t_test = load_data.load_cifar10()
labels_test = np.argmax(t_test, axis=1)


# reshape data
x_train = x_train.reshape((x_train.shape[0], 3, 32, 32))
x_test = x_test.reshape((x_test.shape[0], 3, 32, 32))


# define symbolic Theano variables
x = T.tensor4()
t = T.matrix()


# define model: neural network
def floatX(x):
    return np.asarray(x, dtype=theano.config.floatX)


def init_weights(shape, name):
    return mv_shared(floatX(np.random.randn(*shape) * 0.1), name=name)


def momentum(cost, params, learning_rate, momentum):
    grads = theano.grad(cost, params)
    updates = []

    for p, g in zip(params, grads):
        mparam_i = mv_shared(np.zeros(p.get_value().shape, dtype=theano.config.floatX))
        v = momentum * mparam_i - learning_rate * g
        updates.append((mparam_i, v))
        updates.append((p, p + v))

    return updates


def model(x, w_c1, b_c1, w_c2, b_c2, w_h3, b_h3, w_o, b_o):
    c1 = T.maximum(0, conv2d(x, w_c1) + b_c1.dimshuffle('x', 0, 'x', 'x'))
    p1 = max_pool_2d(c1, (3, 3))

    c2 = T.maximum(0, conv2d(p1, w_c2) + b_c2.dimshuffle('x', 0, 'x', 'x'))
    p2 = max_pool_2d(c2, (2, 2))

    p2_flat = p2.flatten(2)
    h3 = T.maximum(0, T.dot(p2_flat, w_h3) + b_h3)
    p_y_given_x = T.nnet.softmax(T.dot(h3, w_o) + b_o)
    return p_y_given_x

mv.init()
worker_id = mv.worker_id()
# if WORKER_ID == 0, it will be the master woker
is_master_worker = worker_id == 0
workers_num = mv.workers_num()


w_c1 = init_weights((4, 3, 3, 3), name="w_c1")
b_c1 = init_weights((4,), name="b_c1")
w_c2 = init_weights((8, 4, 3, 3), name="w_c2")
b_c2 = init_weights((8,), name="b_c2")
w_h3 = init_weights((8 * 4 * 4, 100), name="w_h3")
b_h3 = init_weights((100,), name="b_h3")
w_o = init_weights((100, 10), name="w_o")
b_o = init_weights((10,), name="b_o")

params = [w_c1, b_c1, w_c2, b_c2, w_h3, b_h3, w_o, b_o]


p_y_given_x = model(x, *params)
y = T.argmax(p_y_given_x, axis=1)

cost = T.mean(T.nnet.categorical_crossentropy(p_y_given_x, t))

updates = momentum(cost, params, learning_rate=0.01, momentum=0.9)


# compile theano functions
train = theano.function([x, t], cost, updates=updates, allow_input_downcast=True)
predict = theano.function([x], y, allow_input_downcast=True)


mv.barrier()

# the non-master process sync values to the master's init values.
if not is_master_worker:
    sync_all_mv_shared_vars()


# train model
batch_size = 50

for i in range(50):
    for start in range(0, len(x_train), batch_size):
        # every process only train batches assigned to itself
        if start / batch_size % workers_num != worker_id:
            continue
        x_batch = x_train[start:start + batch_size]
        t_batch = t_train[start:start + batch_size]
        cost = train(x_batch, t_batch)

        # sync value with multiverso after every batch
        sync_all_mv_shared_vars()

    mv.barrier() # barrier every epoch

    # master will calc the accuracy
    if is_master_worker:
        predictions_test = predict(x_test)
        accuracy = np.mean(predictions_test == labels_test)

        print "epoch %d - accuracy: %.4f" % (i + 1, accuracy)


mv.shutdown()
