# Multiverso Python Binding Benchmark

## Task Description
Perform CIFAR-10 classification with residual networks implementation based on Lasagne.

## Codebase
[Deep_Residual_Learning_CIFAR-10](../examples/theano/lasagne/Deep_Residual_Learning_CIFAR-10.py)

## Hardware
|||
| -------- |:--------:|
|Hosts|1|
|GPU|GeForce GTX TITAN X * 4|
|CPU|Intel(R) Core(TM) i7-5960X CPU @ 3.00GHz  * 1|
|Memory| 128GB |


## Theano settings
Configuration of `~/.theanorc`
```
[global]
device = gpu
floatX = float32

[cuda]
root = /usr/local/cuda-7.5/

[lib]
cnmem = 1
```

## About the Model
|||
| :---- | -----: |
|Total epoch|82|
|Batch size|128|
|Depth|32|
|Learning rate change schedule|Initialized as 0.1, Changed to 0.01 from epoch 41, to 0.001 from epoch 61|
|number of parameters in model|    464,154|


Clarification
- An epoch represents all the processes divide all the data equally and go through them once together.
- A barrier is used at the end of each epoch.
- This experiment doesn't use warm start in ASGD.
- The time to load the data is not considered in the time of the experiment.


# The results
The results of 4 experiments with different configurations are shown as following.

|Short Name | # Process(es) | #GPU(s) per Process | Use multiverso | Sync every X minibatches | Seconds per epoch |  Best model validation accuracy |
| :---- | :-----: | :-----: | :-----: | :-----: | :-----: | :-----: |
| 1P1G0M | 1 | 1 | 0 | -- | 100.02 | 92.61 % |
|1P1G1M1S | 1 | 1 | 1 | 1 | 109.78 | 93.03 % |
|4P1G1M1S | 4 | 1 | 1 | 1 | 29.38 | 92.15 % |
|4P1G1M3S | 4 | 1 | 1 | 3 | 27.46 | 89.61 % |

![accuracy_epoch](./imgs/accuracy_epoch.png)
![accuracy_time](./imgs/accuracy_time.png)
