DMTK-Yarn: Running multiverso job on Yarn clusters.
Tested on Windows 8

Dependency
==========
    1. Yarn 2.6.0
    2. zeromq-4.1.3
	
Build
==========
mvn package
    
Usage
==========
    1. prepare the job as the structure of example(server,worker, hadoop-yarn-applications-dmtk-2.6.0.jar)
	
    2. put all the files of server to the folder server, and provide a command file(start.bat) as an entrance.
	   
    3. put all the files of worker to folder worker, and provide a command file(start.bat) as an entrance.
	
	See the detailed in example.

How it works
==========
	1. After you use the jar to submit ApplicationMaster.
	   ApplicationMaster allocates containers to start servers firstly.
	   It may try several times if servers can not be started.
	   Start-server:ApplicationMaster will copy all the files in folder server to a container,
	   then call the command line (start.bat serverId numWorkers container_ip workerServerPort serverArgs).
	   
	2. After all the servers are started successfully. ApplicationMaster will start workers.	
	   Start-worker:ApplicationMaster will copy all the files in folder worker to a container and
	   generate a endpointlist file(consist of all the servers) to the container,
	   then call the command line (start.bat endpointlistFileName machinelistFileName workerId workerServerPort workerArgs). The format of the data in endpointlistFileName is "serverId ip:workerServerPort", and the format
	   of the data in machinelistFileName is "ip".

	3. If all the workers succeed to exit, then ApplicationMaster exit with code 0;
	   If one of workers and servers failed, then ApplicationMaster with non-zero.
	   
Commandline Options
==========
	-alloctime <arg>   Maximum time to wait while allocating containers,
					   default 120 in seconds
	-appname <arg>     App name
	-c <arg>           Number of cores used by each process. Default 1
	-d                 Enable debug info
	-exectime <arg>    Maximum time to wait while executing app, default
                       2592000 in seconds
	-h                 Print usage
	-jar_file <arg>    AM jar file
	-m <arg>           Memory used by each process (in MB). Default 1024
	-p <arg>           The port am should use to communicate with client
	-q <arg>           Job queue
	-s                 Wait till the ZMQ program exits
	-sa <arg>          The args for server/start.bat
	-sn <arg>          The number of servers. Default 1
	-v                 Enable verbose output
	-wa <arg>          The args for worker/start.bat
	-wn <arg>          The number of workers. Default 1
	-wsp <arg>         The port which servers should use to communicate with
					   workers
