#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include "simple_threadsafe_queue.h"
#include "utils.h"

#include <thread>
#include <iostream>

namespace server
{
using socket_fd = int;
namespace details
{
int set_nonblock(socket_fd fd);
} //namespace details

class Server
{
public:
    explicit Server(const utils::Server_params& params);
    void run();
    void handle_connections();
    void send_response(const socket_fd sock, const std::string& request) const;
    ~Server();
private:
    bool isValidContentRequested(const std::string& content_filename) const;
    std::thread m_worker; //to simplify thread pool im gonna make just one working thread
    socket_fd m_acceptorSocket{ -1 }; 
    utils::Server_params m_serverParams{};
    threadsafe_queue<socket_fd> m_connectionEvents{};
    std::vector<socket_fd> m_slaves{};
};

/*Since requests have very simple structure it will be just small 
class which try to find just file name without any additional libraries
*/
class Request_parser
{
public:
    static std::string get_target_filename(const std::string& http_request);
};

} // namespace server

#endif // SERVER_H_INCLUDED