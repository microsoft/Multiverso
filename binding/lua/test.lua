#!/usr/bin/env lua

require 'torch'

mv = require('multiverso')

local mv_test = torch.TestSuite()
local mv_tester = torch.Tester()

function mv_test.testArray()
    size = 100000
    mv.init()
    tbh = mv.ArrayTableHandler:new(size)
    mv.barrier()

    for i = 1, 1000 do
        print(tbh:get()[{{1, 10}}])
        tbh:add(torch.range(1, size))
        tbh:add(torch.range(1, size))
        mv.barrier()
    end

    mv.shutdown()
end

mv_tester:add(mv_test)
mv_tester:run()
