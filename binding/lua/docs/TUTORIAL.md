# Integrate multiverso into torch project

## Setup

1. Install CUDA, cuDNN, Torch and Torch cuDNN bindings according to this
   [guide](https://github.com/Microsoft/fb.resnet.torch/blob/multiverso/INSTALL.md).
1. Install Multiverso shared object by referring to the
   [Build](../../../README.md#build) instruction of multiverso project.
1. Install multiverso torch package by referring to the
   [Installation](../README.md#Installation) instruction of multiverso torch/lua
   binding.

## Package Initialization

Load and initialize multiverso package and then get some useful parameters at
the beginning of the whole project.

```lua
-- Load multiverso.
local multiverso = require 'multiverso'
-- Init multiverso.
multiverso.init()
-- Get total number of workers.
multiverso.num_workers = multiverso.num_workers()
-- Get the id for current worker.
multiverso.worker_id = multiverso.worker_id()
-- Easy access to check whether this is master worker.
multiverso.is_master = multiverso.worker_id == 0
```

## Table Handler Initialization

Create a Table Handlder as an interface for syncing issues.

#### Assumptions and Clarifications:

1. `model` variable is a
   [Module](https://github.com/torch/nn/blob/master/doc/module.md#module)
   class in `torch.nn` package to build neural networks.
1. `ArrayTableHandler` is used for example as it satisfies most cases.
1. Actually, we can sync any variables (`tables` in Lua or `Tensors` in torch)
   with multiverso but model syncing is used as example here cause it is the
   most common user case.
1. During the initialization, we need to specify the exact size to sync.

```lua
-- Get static params and gradParams from model variable.
local params, gradParams = model.getParameters()
-- Create ArrayTableHandler for syncing parameters.
local tbh = multiverso.ArrayTableHandler:new(params:size(1))
```

## Model Initialization

Before actual training, we also need to make sure each worker has the same
initial model for better training performance. Two strageties are recommended
here:

### Master stragety

Only the master worker sync its model to the server, other workers copy the
model of the master worker from the server.

```lua
if multiverso.is_master then
    -- Only master worker will set the initial value.
    tbh:add(params)
    -- Set a barrier for other workers to wait.
    multiverso.barrier()
else
    -- Wait the master worker to finish setting.
    multiverso.barrier()
    -- Get the initial model from the server.
    params:copy(tbh:get())
end
```

### Equality stargety

All workers contribute equally to the initial model on the server and then
fetch same initial models after finish the contributing phase.

```lua
-- Contribute equally to the server.
tbh:add(params / multiverso.num_workers)
-- Wait for finshing the contributing phase.
multiverso.barrier()
-- Fetch same initial model.
params:copy(tbh:get())
```

## Sync Parameters

During training or any other places we want to sync something. Two steps are
needed:

1. Add the gradients (delta value) to the server.
1. Fetch the newest value from the server.

#### Assumptions and Clarifications:

1. `learingRate` variable is the learing rate maintained by ourselves.
1. Only gradients (delta value) should be passed to the table handler.
1. This step should overwrite other changes to `params` variable, so we use
   `params:copy()` here.

```lua
-- Add the gradients (delta value) to the server.
tbh:add(learingRate * gradParams)
-- Fetch the newest value from the server.
params:copy(tbh:get())
```

## (Optional) Perform only in master

Sometimes, we want to do somehing like log printing or validation. These kind
of procedures should only performed in the master worker as we won't be able to
see any result from other workers.

```lua
if multiverso.is_master then
    -- Do something like print or validation here.
end
```

## (Optional) Synchronize all workers

Sometimes, we want all workers to have the same state after they are trained
distributively after several epochs, e.g., each 10 epochs.

#### Assumptions and Clarifications:

1. `epoch` variable is the number of current epoch.
2. This should perform between the first and the second step in sync phase.

```lua
-- Add the gradients (delta value) to the server.
tbh:add(learingRate * gradParams)
-- Synchronize all workers each several epochs.
if epoch % 10 == 0:
    multiverso.barrier()
end
-- Fetch the newest value from the server.
params:copy(tbh:get())
```

## Shutdown multiverso

After finishing training, just remember to shutdown before exit.

```lua
multiverso.shutdown()
-- This should be the end of the whole project.
```

## Happy using multiverso (torch/lua binding)
