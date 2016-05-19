#!/usr/bin/env lua

mv = {}

ffi = require('ffi')
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
    data = {}
    for i=1, tonumber(self._size) do
        data[i] = cdata[i - 1]
    end
    return torch.Tensor(data)
end

function mv.ArrayTableHandler:add(data)
    cdata = ffi.new("float[?]", self._size)
    for i=1, tonumber(self._size) do
        cdata[i - 1] = data[i]
    end
    libmv.MV_AddArrayTable(self._handler[0], cdata, self._size)
end

return mv
