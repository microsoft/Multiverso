#!/usr/bin/env lua

mv = {}

ffi = require('ffi')
util = require('util')

ffi.cdef[[
    typedef void* TableHandler;
    void MV_Init(int* argc, char* argv[]);
    void MV_ShutDown();
    void MV_Barrier();
    int MV_NumWorkers();
    int MV_WorkerId();
    int MV_ServerId();

    // Array Table
    void MV_NewArrayTable(int size, TableHandler* out);
    void MV_GetArrayTable(TableHandler handler, float* data, int size);
    void MV_AddArrayTable(TableHandler handler, float* data, int size);

    // Matrix Table
    void MV_NewMatrixTable(int num_row, int num_col, TableHandler* out);
    void MV_GetMatrixTableAll(TableHandler handler, float* data, int size);
    void MV_AddMatrixTableAll(TableHandler handler, float* data, int size);
    void MV_GetMatrixTableByRows(TableHandler handler, int row_ids[], int row_ids_n, int num_col,  float** data);
    void MV_AddMatrixTableByRows(TableHandler handler, float* data, int num_col, int row_ids[], int row_ids_n);
]]

package.cpath = "../../build/src/?.so;" .. package.cpath
libmv_path = package.searchpath('libmultiverso', package.cpath, '')
libmv = ffi.load(libmv_path)

function mv.init(args)
    args = args or {}
    argc = ffi.new("int[1]", #args)
    argv = ffi.new("char*[?]", #args)
    for i = 1, #args do
        argv[i - 1] = ffi.new("char[1]")
        ffi.copy(argv[i - 1], args[i])
    end
    libmv.MV_Init(argc, argv)
end

function mv.barrier()
    libmv.MV_Barrier()
end

function mv.shutdown()
    libmv.MV_ShutDown()
end

function mv.num_workers()
    return libmv.MV_NumWorkers()
end

function mv.worker_id()
    return libmv.MV_WorkerId()
end

function mv.server_id()
    return libmv.MV_ServerId()
end

mv.ArrayTableHandler = {}

function mv.ArrayTableHandler:new(size)
    tbh = {}
    size = size or 0
    setmetatable(tbh, self)
    self.__index = self
    tbh._handler = ffi.new("TableHandler[1]")
    tbh._size = ffi.new("int", size)
    libmv.MV_NewArrayTable(
        tbh._size,
        tbh._handler
    )
    return tbh
end

function mv.ArrayTableHandler:get()
    cdata = ffi.new("float[?]", self._size)
    libmv.MV_GetArrayTable(self._handler[0], cdata, self._size)
    return util.cdata2array(cdata, tonumber(self._size))
end

function mv.ArrayTableHandler:add(data)
    cdata = util.tensor2cdata(data)
    libmv.MV_AddArrayTable(self._handler[0], cdata, self._size)
end

mv.MatrixTableHandler = {}

function mv.MatrixTableHandler:new(num_row, num_col)
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

function mv.MatrixTableHandler:get(row_ids)
    if row_ids == nil then
        cdata = ffi.new("float[?]", self._size)
        libmv.MV_GetMatrixTableAll(self._handler[0], cdata, self._size)
        data = util.cdata2array(cdata, tonumber(self._size))
        return torch.reshape(data, tonumber(self._num_row), tonumber(self._num_col))
    else
        crow_ids = util.tensor2cdata(row_ids, 'int')
        crow_ids_n = ffi.new("int", #row_ids)
        cdata = ffi.new("float*[?]", #row_ids * self._num_col)
        lib.MV_GetMatrixTableByRows(self._handler[0], crow_ids, crow_ids_n, self._num_col, cdata)
        data = util.cdata2array(cdata, tonumber(self._size))
        return torch.reshape(data, #row_ids, self._num_col)
    end
end

function mv.MatrixTableHandler:add(data, row_ids)
    if row_ids == nil then
        cdata = util.tensor2cdata(data)
        libmv.MV_AddMatrixTableAll(self._handler[0], cdata, self._size)
    else
        data = torch.reshape(data, #row_ids, tonumber(self._num_col))
        crow_ids = util.tensor2cdata(row_ids, 'int')
        crow_ids_n = ffi.new("int", #row_ids)
        cdata = util.tensor2cdata(data)
        libmv.MV_AddMatrixTableByRows(self._handler[0], cdata, self._num_col,
                                      crow_ids, crow_ids_n)
    end
end

return mv
