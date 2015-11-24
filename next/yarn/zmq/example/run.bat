call yarn jar hadoop-yarn-applications-zmq-2.6.0.jar -jar_file hadoop-yarn-applications-zmq-2.6.0.jar -d -v -sa serarg -wa workarg -wsp %1 -p %2 -s -sn %3 -wn %4
echo run.bat-exit-Code%errorlevel%
exit %errorlevel%