## Installation on Windows

1. Install the [ZeroMQ](http://zeromq.org/intro:get-the-software) from [http://miru.hk/archive/ZeroMQ-4.0.4~miru1.0-x64.exe](http://miru.hk/archive/ZeroMQ-4.0.4~miru1.0-x64.exe)
2. Download the C++ wrapper of ZeroMQ [zmq.hpp](https://github.com/zeromq/cppzmq), and move zmq.hpp into ```$(ZMQDir)/include``` folder.
3. Install the [msmpi v5](https://msdn.microsoft.com/en-us/library/bb524831(v=vs.85).aspx) , include the [MSMPI_Setup](http://download.microsoft.com/download/3/7/6/3764A48C-5C4E-4E4D-91DA-68CED9032EDE/MSMpiSetup.exe) and [MSMPI_SDK](http://download.microsoft.com/download/3/7/6/3764A48C-5C4E-4E4D-91DA-68CED9032EDE/msmpisdk.msi). Then set the ```$(MsmpiDir)/bin``` folder in the system path(For use of mpiexec.exe and smpd.exe).
4. Copy related ```include``` and ```lib``` file into third_party folder.
5. Open the ```multiverso.sln```, and build all projects.
