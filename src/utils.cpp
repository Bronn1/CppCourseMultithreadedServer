#include "utils.h"

#include <getopt.h>
#include <iostream>

namespace utils
{
Args_parser::Args_parser(int argc, char** argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "h:p:d:")) != -1) {
        switch (opt) {
        case 'h':
        {
            std::string ip = std::string(optarg);
            if(!ip.empty()){
                m_server_params.ip = std::move(ip);
            }
            break;
        }
        case 'p':
        {
            int port = std::atoi(optarg);
            if(port != -1){
                m_server_params.port = port;
            }
            break;
        }
        case 'd':
        {
            std::string dir = std::string(optarg);
            if(!dir.empty()){
                m_server_params.home_dir = std::move(dir);
            }
            break;
        }
        default: 
            std::cerr <<  "Usage: "<< std::string("" + char(opt)).c_str() <<"  [-t nsecs] [-n] name\n";
            exit(EXIT_FAILURE);
        }
    }
}

} // namespace utils