# Multiverso Torch Binding Benchmark

## Task Description

Perform CIFAR-10 classification with torch resnet implementation.

## Codebase

[Microsoft/fb.resnet.torch multiverso branch](https://github.com/Microsoft/fb.resnet.torch/tree/multiverso)

## Setup
Please follow [this guide](https://github.com/Microsoft/multiverso/wiki/Multiverso-Torch-Lua-Binding) to setup your environment.

## Hardware

- **Hosts** : 1
- **GPU** : GeForce GTX TITAN X * 4
- **CPU** : Intel(R) Core(TM) i7-5960X CPU @ 3.00GHz
- **Memory** : 128GB

## Common settings

- batchSize 128
- depth 32
- nEpochs 164
- learningRate 0.1(epoch <= 80), 0.01(81 <= epoch <= 121), 0.001(121 <= epoch)

## Clarification for multiverso settings

- The train data is divided evenly to each worker.
- Master strategy is used to warm up the initial model.
- Workers sync after each batch and has a barrier after each epoch.

## Results

| Code Name | #Process(es) | #GPU(s) per Process | Use multiverso | Seconds per epoch | Best Model |
| :-------: | :----------: | :-----------------: | :------------: | :---------------: | :--------: |
| 1P1G0M    | 1            | 1                   | 0              | 20.366            | 92.712 %   |
| 1P4G0M    | 1            | 4                   | 0              | 10.045            | 92.296 %   |
| 4P1G1M    | 4            | 1                   | 1              |  6.303            | 91.390 %   |

![top1error_vs_epoch](https://raw.githubusercontent.com/Microsoft/multiverso/master/binding/lua/docs/imgs/top1error_vs_epoch.png)
![top1error_vs_runningtime](https://raw.githubusercontent.com/Microsoft/multiverso/master/binding/lua/docs/imgs/top1error_vs_runningtime.png)
