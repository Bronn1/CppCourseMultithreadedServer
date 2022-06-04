#include "server.h"
#include "i_http_response.h"

#include <stdexcept>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fstream>

constexpr int kMaxConnections = 100;

namespace server
{
namespace details
{
int set_nonblock(socket_fd fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, (unsigned) flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIONBIO, &flags);
#endif
} 
}// namespace details

Server::Server(const utils::Server_params& params) : m_serverParams(params)
{
    m_acceptorSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_acceptorSocket < 0){ 
        throw std::runtime_error("Cannot create acceptor socket: " +  std::string(std::strerror(errno)));
    }
    int opt = 1;
    if (setsockopt(m_acceptorSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error("Cannot set sockopt on acceptor socket: "+   std::string(std::strerror(errno)));
    }

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(m_serverParams.port);
    sock_addr.sin_addr.s_addr = inet_addr(m_serverParams.ip.c_str());

    if(bind(m_acceptorSocket, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0){
        throw std::runtime_error("Cannot bind acceptor socket: " + std::string(std::strerror(errno)));
    }

    if(details::set_nonblock(m_acceptorSocket) < 0) {
        throw std::runtime_error("Cannot set non blocking mode: "+ std::string(std::strerror(errno)));
    }

    if(listen(m_acceptorSocket, SOMAXCONN) < 0) {
        throw std::runtime_error("Cannot listen on acceptor socket: " + std::string(std::strerror(errno)));
    }

    std::cerr << "Max available threads: " << std::thread::hardware_concurrency() << '\n'; 
    m_worker = std::thread(&Server::handle_connections, this);
}

void Server::handle_connections()
{
    std::cerr << "Worker thread successfully started..\n";
    constexpr int kReadBufMax = 1024;
    std::string recieved_msg{""};
    int bytes_read = 0;
    while(true){
        auto sock_with_event = m_connectionEvents.wait_and_pop();
        recieved_msg.clear();
        char read_buf[kReadBufMax];
        memset(read_buf, '\0', kReadBufMax);

        int bytes = recv(*sock_with_event, read_buf, kReadBufMax - 1, 0);
        bytes_read += bytes;
        if(bytes <= 0 && errno != EAGAIN){
            std::cerr << "Connection closed or error ocurred: " << std::strerror(errno) << '\n';
            shutdown(*sock_with_event, SHUT_RDWR);
            close(*sock_with_event);
        }
        else if(errno == EAGAIN){
            send_response(*sock_with_event, recieved_msg);
            //std::cerr << "Msg received EAGAIN: " << recieved_msg << '\n';  
        }
        else if(bytes > 0) {
            recieved_msg.append(read_buf, bytes_read);
            send_response(*sock_with_event, recieved_msg);
            //std::cerr << "Msg received: \n" << recieved_msg << '\n'; 
        }
    }
}

void Server::send_response(const socket_fd sock, const std::string& request) const
{
    std::string content_file = Request_parser::get_target_filename(request);
    std::string content_filename = m_serverParams.home_dir + "/" + content_file;
    http_response_type resp_type = isValidContentRequested(content_filename) ? 
                                   http_response_type::OK :
                                   http_response_type::ERROR;

    std::string response =  Form_response_factory::create_response(resp_type, content_filename);
    //std::cout << response << "res\n\n";
    int bytes_sent = send(sock, response.c_str(), response.size(), 0);
    if(bytes_sent < 0){
        std::cerr << "error cannot send response: " << std::strerror(errno) << '\n';
    }
}

void Server::run()
{
    std::cerr << "Running server...\n";
    int EPool = epoll_create1(0);
    if (EPool < 0){
        throw std::runtime_error("Cannot create epoll: "+ std::string(std::strerror(errno)));
    }
    struct epoll_event event;
    event.data.fd = m_acceptorSocket;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(EPool, EPOLL_CTL_ADD, m_acceptorSocket, &event);

    while(true){
        struct epoll_event events[kMaxConnections];
        int events_amount = epoll_wait(EPool, events, kMaxConnections, -1);
        for(int  i = 0; i < events_amount; ++i){
            if( (events[i].events & EPOLLERR ) || events[i].events & EPOLLHUP ){
                throw std::runtime_error("EPOLL ERROR caught\n");
            }
            if(events[i].data.fd== m_acceptorSocket){
                socket_fd slaveSock = accept(m_acceptorSocket, 0 , 0);
                if(details::set_nonblock(m_acceptorSocket) < 0) {
                    throw std::runtime_error("Cannot set non blocking mode: "+ std::string(std::strerror(errno)));
                }
                struct epoll_event event;
                event.data.fd = slaveSock;
                event.events = EPOLLIN | EPOLLET;
                epoll_ctl(EPool, EPOLL_CTL_ADD, slaveSock, &event);
                m_slaves.push_back(slaveSock);

            } else{
                m_connectionEvents.push(events[i].data.fd);
            }
        }
    }

    close(EPool);
}

Server::~Server()
{
    if(m_acceptorSocket > 0) {
        close(m_acceptorSocket);
    }
    for(const auto& sock: m_slaves){
      close(sock);  
    }
    if(m_worker.joinable()){
        m_worker.join();
    }
    
}

bool Server::isValidContentRequested(const std::string& content_filename) const
{
    std::ifstream f(content_filename.c_str());
    return f.good();
}

std::string Request_parser::get_target_filename(const std::string& http_request)
{
    const std::string mainPage{"index.html"};
    //  magic numbers there but this part is not  
    // purpose of this exam so i did as fast as possible
    std::cout << http_request << std::endl;
    std::size_t pos1 = http_request.find("GET /");
    std::size_t pos2 = http_request.find(" HTTP/1");
    if (pos1 == std::string::npos || pos2 == std::string::npos) return "";
    std::string ind = http_request.substr(pos1 + 5, pos2 - pos1 - 5);
    if (ind.size() == 0) return mainPage;

    auto pos = ind.find('?');
    if (pos == std::string::npos)
        return ind;
    else
        return ind.substr(0, pos);
    //std::string target_filename{""};
    //size_t posEnd = http_request.find("\r\n\r\n");
    //size_t lenTerm = 4;
    //if (posEnd == std::string::npos) {
    //    posEnd = http_request.find("\n\n");
    //    lenTerm = 2;
    //}
    //if (posEnd != std::string::npos) {
    //    std::string stRequest = http_request.substr(0, posEnd + 2);
    //    std::string stFirstLine = stRequest.substr(0, stRequest.find("\n"));
    //    //std::string http_request = http_request.substr(posEnd + lenTerm);
    //    std::string pathRequest;
//
//
    //    if (stRequest.substr(0, 3) == "GET") {
    //        size_t posHttp = stFirstLine.find("HTTP/1.");
    //        if (posHttp != std::string::npos)
    //        {
    //            pathRequest = stFirstLine.substr(4, posHttp - 5);
    //            target_filename = pathRequest;
    //        }
    //    }
//
    //    if (target_filename.empty()) {
    //        std::cerr << "ERROR: Can't parse the request" << std::endl;;
    //    }
    //        
    //}
//
    //return target_filename;
}


} //namespace server