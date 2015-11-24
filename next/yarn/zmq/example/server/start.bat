dir
echo %*
set PATH=%PATH%;%~dp0\bin
multiverso_server.exe -serverid %1 -workers %2 -endpoint %3
rem ping 127.0.0.1 -w 120000
echo exit-Code%errorlevel%
exit %errorlevel%