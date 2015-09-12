#include <sstream>
#include "cl_parser.h"

namespace multiverso
{
    CommandLineParser::CommandLineParser(int argc, char *argv[])
    {
        for (int i = 1; i < argc; ++i)
        {
            if (argv[i][0] == '-')
            {
                if (i + 1 < argc && argv[i + 1][0] != '-')
                {
                    args_[argv[i]] = argv[i + 1];
                    ++i;
                }
                else
                {
                    args_[argv[i]] = "";
                }
            }
        }
    }

    CommandLineParser::~CommandLineParser() {}
}
