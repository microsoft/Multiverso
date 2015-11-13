#ifndef _MULTIVERSO_CL_PARSER_H_
#define _MULTIVERSO_CL_PARSER_H_

/*!
 * \file cl_parser.h
 * \brief Defines a simple command line argument parser
 * \author feiyan
 */

#include <string>
#include <sstream>
#include <unordered_map>

namespace multiverso
{
    /*!
     * \beirf The CommandLineParser class is a simple command line argument 
     *        parser. The command line arguments shoud consists of some 
     *        argument items. Each item has the format of either "argument_key"
     *        or "argument_key argument_value" in which the argument_key is a 
     *        case-sensitive string starting with "-". For example, "-config" 
     *        or "-id 0" are both correct argument items.
     */
    class CommandLineParser
    {
    public:
        CommandLineParser(int argc, char *argv[]);
        ~CommandLineParser();

        /*! \brief Returns whether the specific key exists. */
        bool HasKey(std::string key) { return args_.count(key) > 0; }

        /*! \beirf Returns the argument value of the specific key. */
        template <typename T>
        T GetValue(std::string key)
        {
            std::stringstream ss(args_[key]);
            T ret;
            ss >> ret;
            return ret;
        }

    private:
        std::unordered_map<std::string, std::string> args_;
    };

    // template <>
    // std::string CommandLineParser::GetValue<std::string>(std::string key)
    // { 
        // return args_[key]; 
    // }
}

#endif // _MULTIVERSO_CL_PARSER_H_
