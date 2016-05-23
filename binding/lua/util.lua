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

function util.cdata2matrix(data, num_row, num_col)
    data = torch.Tensor(num_row, num_col)
    for i=1, num_row do
        for j=1, num_col do
            data[i][j] = cdata[i - 1][j - 1]
        end
    end
    return data
end

function util.matrix2cdata(data, num_row, num_col, cdata_type)
    cdata_type = cdata_type or "float*[?]"
    cdata = ffi.new(cdata_type, num_row)
    for i=1, num_row do
        cdata[i - 1] = ffi.new("float[?]", num_col)
        for j=1, num_col do
            cdata[i - 1][j - 1] = data[i][j]
        end
    end
    return cdata
end

function util.Set(list)
  local set = {}
  for _, l in ipairs(list) do set[l] = true end
  return set
end

return util
