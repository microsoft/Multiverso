local ffi = require 'ffi'
local util = require('multiverso.util')

local tbh = torch.class('MatrixTableHandler')

ffi.cdef[[
    void MV_NewMatrixTable(int num_row, int num_col, TableHandler* out);
    void MV_GetMatrixTableAll(TableHandler handler, float* data, int size);
    void MV_AddMatrixTableAll(TableHandler handler, float* data, int size);
    void MV_AddAsyncMatrixTableAll(TableHandler handler, float* data, int size);
    void MV_GetMatrixTableByRows(TableHandler handler, float* data, int size, int row_ids[], int row_ids_n);
    void MV_AddMatrixTableByRows(TableHandler handler, float* data, int size, int row_ids[], int row_ids_n);
    void MV_AddAsyncMatrixTableByRows(TableHandler handler, float* data, int size, int row_ids[], int row_ids_n);
]]

function tbh:new(num_row, num_col)
    tbh = {}
    num_row = num_row or 0
    num_col = num_col or 0
    setmetatable(tbh, self)
    self.__index = self
    tbh._handler = ffi.new("TableHandler[1]")
    tbh._num_row = ffi.new("int", num_row)
    tbh._num_col = ffi.new("int", num_col)
    tbh._size = ffi.new("int", num_row * num_col)
    libmv.MV_NewMatrixTable(
        tbh._num_row,
        tbh._num_col,
        tbh._handler
    )
    return tbh
end

function tbh:get(row_ids)
    if row_ids == nil then
        cdata = ffi.new("float[?]", self._size)
        libmv.MV_GetMatrixTableAll(self._handler[0], cdata, self._size)
        data = util.cdata2tensor(cdata, tonumber(self._size))
        return torch.reshape(data, tonumber(self._num_row), tonumber(self._num_col))
    else
        cdata = ffi.new("float[?]", #row_ids * self._num_col)
        crow_ids = util.tensor2cdata(row_ids, 'int')
        crow_ids_n = ffi.new("int", #row_ids)
        libmv.MV_GetMatrixTableByRows(self._handler[0], cdata,
                                      crow_ids_n * self._num_col,
                                      crow_ids, crow_ids_n)
        data = util.cdata2tensor(cdata, tonumber(#row_ids * self._num_col))
        return torch.reshape(data, #row_ids, tonumber(self._num_col))
    end
end

function tbh:add(data, row_ids, sync)
    sync = sync or false
    cdata = util.tensor2cdata(data)
    if row_ids == nil then
        if sync then
            libmv.MV_AddMatrixTableAll(self._handler[0], cdata, self._size)
        else
            libmv.MV_AddAsyncMatrixTableAll(self._handler[0], cdata, self._size)
        end
    else
        crow_ids = util.tensor2cdata(row_ids, 'int')
        crow_ids_n = ffi.new("int", #row_ids)
        if sync then
            libmv.MV_AddMatrixTableByRows(self._handler[0], cdata,
                                          crow_ids_n * self._num_col,
                                          crow_ids, crow_ids_n)
        else
            libmv.MV_AddAsyncMatrixTableByRows(self._handler[0], cdata,
                                          crow_ids_n * self._num_col,
                                          crow_ids, crow_ids_n)
        end
    end
end

return tbh
