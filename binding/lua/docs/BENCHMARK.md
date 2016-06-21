#Multiverso Torch Binding Benchmark

## Task Description

Perform CIFAR-10 classification with torch resnet implementation.

## Codebase

[Microsoft/fb.resnet.torch multiverso branch](https://github.com/Microsoft/fb.resnet.torch/tree/multiverso)

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

| Code Name | #Process | #GPU per Process | Use multiverso | Seconds per epoch | Best Model            |
| :-------: | :------: | :--------------: | :------------: | :---------------: | :-------------------: |
| 1P1G0M    | 1        | 1                | 0              | 20.69             | top1:7.288 top5:0.218 |
| 1P4G0M    | 1        | 4                | 0              | 10.04             | top1:7.704 top5:0.289 |
| 4P1G1M    | 4        | 1                | 1              |  6.30             | top1:8.849 top5:0.391 |

![top1error_vs_epoch](./imgs/top1error_vs_epoch.png)
![top5error_vs_epoch](./imgs/top5error_vs_epoch.png)
![top1error_vs_runningtime](./imgs/top1error_vs_runningtime.png)
![top5error_vs_runningtime](./imgs/top5error_vs_runningtime.png)
