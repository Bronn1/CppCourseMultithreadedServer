#include "i_http_response.h"

#include <sstream>
#include <fstream>

namespace server
{

Http_200_response::Http_200_response(const std::string& filename)
{

    std::ifstream f(filename.c_str());
    if (f.good())
    {
        std::stringstream buffer;
        buffer << f.rdbuf();
        m_body = buffer.str();
    }
}
std::string Http_200_response::getResponse() const
{
    std::stringstream ss;
    ss << "HTTP/1.0 200 OK";
    ss << "\r\n";
    ss << "Content-length: ";
    ss << m_body.size();
    ss << "\r\n";
    ss << "Content-Type: text/html";
    ss << "\r\n";
    // ss << "Connection: close\r\n";
    ss << "\r\n";
    ss << m_body;
    
    return ss.str();
}

std::string Http_404_response::getResponse() const  
{
    std::stringstream ss;
    // Create a result with "HTTP/1.0 404 NOT FOUND"
    ss << "HTTP/1.0 404 NOT FOUND";
    ss << "\r\n";
    ss << "Content-length: ";
    ss << 0;
    ss << "\r\n";
    ss << "Content-Type: text/html";
    ss << "\r\n\r\n";
    return ss.str();
}

std::string Form_response_factory::create_response(const http_response_type type, const std::string& content_filename)
{
    Http_404_response response404{};
    std::string response = response404.getResponse();
    switch (type) {
    case http_response_type::OK:
    {
        Http_200_response response200{content_filename};
        response = response200.getResponse();
        break;
    }
    case http_response_type::ERROR:
    {
        response = response404.getResponse();
        break;
    }
    }

    return response;
}
} //namespace server
