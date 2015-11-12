// The entrance cpp file of Multiverso Server component.

#include <iostream>
#include <string>
#include <algorithm>
#include "log.h"
#include "server.h"
#include "cl_parser.h"
#include "endpoint_list.h"

// Writes the help information.
void Help()
{
    std::string help_info = "";
    help_info = help_info
        + "Commandline parameters:\n"
        + "-serverid <id> -workers <num_of_workers> "
        + "[-config <filename> | -endpoint <ip:port>] "
        + "[-logfile <filename>] [-loglevel <level>]\n\n"
        + "-serverid <id> -- The server id\n"
        + "-workers <num_of_workers> -- Number of worker processes\n"
        + "-config <filename> -- A server ZMQ socket endpoint list file\n"
        + "-endpoint <ip:port> -- The ZMQ socket endpoint of the current server\n"
        + "-logfile <filename> -- The log filename\n"
        + "-loglevel <level> -- The log level\n";
    printf(help_info.c_str());
}

// Returns the LogLevel according to the string.
multiverso::LogLevel GetLogLevel(std::string level)
{
    std::transform(level.begin(), level.end(), level.begin(), ::tolower);
    if (level == "debug")
    {
        return multiverso::LogLevel::Debug;
    }
    else if (level == "info")
    {
        return multiverso::LogLevel::Info;
    }
    else if (level == "error")
    {
        return multiverso::LogLevel::Error;
    }
    else if (level == "fatal")
    {
        return multiverso::LogLevel::Fatal;
    }
    return multiverso::LogLevel::Info;
}

// Main entrance function
int main(int argc, char *argv[])
{
    multiverso::CommandLineParser cl_parser(argc, argv); // parse the commandline
    if (cl_parser.HasKey("-help")) // output the help information
    {
        Help();
        return 0;
    }

    int server_id = cl_parser.GetValue<int>("-serverid");
    int num_workers = cl_parser.GetValue<int>("-workers");

    // config the logging tool
#if defined (_MULTIVERSO_DEBUG_)
    multiverso::Log::ResetLogLevel(multiverso::LogLevel::Debug);
#endif
    if (cl_parser.HasKey("-logfile"))
    {
        std::string logfile = cl_parser.GetValue<std::string>("-logfile");
        multiverso::Log::ResetLogFile(logfile);
        multiverso::Log::Info("Server %d: Reset log file to %s\n", 
            server_id, logfile.c_str());
    }
    if (cl_parser.HasKey("-loglevel"))
    {
        std::string level = cl_parser.GetValue<std::string>("-loglevel");
        multiverso::Log::ResetLogLevel(GetLogLevel(level));
        multiverso::Log::Info("Server %d: Reset log level to %s\n", 
            server_id, level.c_str());
    }

    // get the ZMQ socket endpoint of this server
    std::string endpoint = "tcp://";
    if (cl_parser.HasKey("-config"))
    {
        multiverso::EndpointList
            endpoint_list(cl_parser.GetValue<std::string>("-config"));
        endpoint += endpoint_list.GetEndpoint(server_id);
    }
    else if (cl_parser.HasKey("-endpoint"))
    {
        endpoint += cl_parser.GetValue<std::string>("-endpoint");
    }

    multiverso::Server server(server_id, num_workers, endpoint);
    server.WaitToComplete();

    return 0;
}
