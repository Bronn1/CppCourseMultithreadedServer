#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <string>

namespace utils
{
struct Server_params
{
	std::string ip{"127.0.0.1"};
	std::string home_dir{"/tmp"};
	int port{1235};
};

class Args_parser
{
public:
	Args_parser(int argc, char** argv);

	Server_params getServerParams() const { return m_server_params; }

private:
	Server_params m_server_params{};
};


} // namespace utils

#endif // UTILS_H_INCLUDED