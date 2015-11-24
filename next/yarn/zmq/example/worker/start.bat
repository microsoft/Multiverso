dir
echo %*
set PATH=%PATH%;%~dp0\bin
app.exe %1
rem ping 127.0.0.1 -w 60000
echo exit-Code%errorlevel%
exit %errorlevel%