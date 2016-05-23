#!/usr/bin/env lua

util = {}

ffi = require('ffi')

util.tensor_type = {
    ['unsigned char'] = 'torch.ByteTensor',
    ['char'] = 'torch.CharTensor',
    ['short'] = 'torch.ShortTensor',
    ['int'] = 'torch.IntTensor',
    ['long'] = 'torch.LongTensor',
    ['float'] ='torch.FloatTensor',
    ['double'] = 'torch.DoubleTensor'
}

function util.tensor2cdata(data, data_type)
    data_type = data_type or 'float'
    tensor_type = util.tensor_type[data_type]
    return torch.Tensor(data):contiguous():type(tensor_type):data()
end

function util.cdata2array(cdata, size)
    data = torch.Tensor(size)
    for i=1, size do
        data[i] = cdata[i - 1]
    end
    return data
end

function util.cdata2matrix(data, num_row, num_col)
    data = torch.Tensor(num_row, num_col)
    for i=1, num_row do
        for j=1, num_col do
            data[i][j] = cdata[i - 1][j - 1]
        end
    end
    return data
end

function util.Set(list)
  local set = {}
  for _, l in ipairs(list) do set[l] = true end
  return set
end

return util
