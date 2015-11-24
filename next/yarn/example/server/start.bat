dir
echo %*
set PATH=%PATH%;%~dp0\bin
rem start.bat serverId numWorkers ip workerServerPort serverArgs
multiverso_server.exe -serverid %1 -workers %2 -endpoint %3:%4
rem ping 127.0.0.1 -w 120000
echo exit-Code%errorlevel%
exit %errorlevel%