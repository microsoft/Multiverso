# Requirements
I presume you followed the [README](../../README.md) and have build multiverso successfully.


# Run tests
```
make test
```


# Run logistic regression example with multi-process
```
mpirun -np 4 python ./examples/theano/logistic_regression.py
```



# How to write python code with multiverso
You can start with the [test example][./multiverso/test.py]



# How to use multiverso in theano
First, make sure you have understood `mv.init()`, `mv.shutdown()` and `mv.barrier()` mentioned in last section.  You should add the same functions in your theano python script.

In theano, parameters usually stored in sharedVariables.

For example, you may find code like this in a theano script
```
self.W = theano.shared(
    value=numpy.zeros(
        (n_in, n_out),
        dtype=theano.config.floatX
    ),
    name='W',
    borrow=True
)
```

If you want to use multiverso, you can modify them like this.
```
from multiverso.theano_ext import sharedvar
W = sharedvar.mv_shared(
    value=numpy.zeros(
        (n_in, n_out),
        dtype=theano.config.floatX
    ),
    name='W',
    borrow=True
)

# build the model

# train the modle

# When you are ready to add the delta of the variable to parameter server and sync the latest value, you can run this function
W.mv_sync()


# If you want to sync all variables created by `sharedvar.mv_shared`, you can use this function
sharedvar.sync_all_mv_shared_vars()
```

If your program will run in multi-process and you want to initialize your parameters, you should initialize your shared variables with the value you want only in one process and initialize them with zero in other processes.


If you don't use shared variables to store and update the parameters, you can still use the `add` and `get` functions to sync parameters


# How to use multiverso in lasagne
First, make sure you have understood the last section.

Lasagne provides many functions to help you build models easilier. Multiverso python binding provides a convenient manager to make managing and synchronizing the variables easilier.

You can use code like this to manage your parameters
```
from multiverso.theano_ext.lasagne_ext import param_manager

network = build_model()  # build_model is a function you implement to build model
mvnpm = param_manager.MVNetParamManager(network, is_master_worker)  # is_master_worker is true only in one process. When it is true, the process will initialize the parameters

# training the model

# When you are ready to add the delta of the variable in this model to the parameter server and get the latest value, you can run this function
mvnpm.update_all_param()
```
