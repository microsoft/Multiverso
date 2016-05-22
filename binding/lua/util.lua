#!/usr/bin/env lua

util = {}

ffi = require('ffi')

function util.cdata2array(cdata, size)
    data = torch.Tensor(size)
    for i=1, size do
        data[i] = cdata[i - 1]
    end
    return data
end

function util.array2cdata(data, size, cdata_type)
    cdata_type = cdata_type or "float[?]"
    cdata = ffi.new(cdata_type, size)
    for i=1, size do
        cdata[i - 1] = data[i]
    end
    return cdata
end

return util
