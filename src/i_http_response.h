#ifndef I_HTTP_RESPONSE_H_INCLUDED
#define I_HTTP_RESPONSE_H_INCLUDED

#include <string>

namespace server
{

enum class  http_response_type
{
    OK = 200,
    ERROR = 404
};

class I_http_response
{
public:
    virtual std::string getResponse() const = 0;
    virtual ~I_http_response() = default;
};

class Http_200_response final : public I_http_response
{
public:
    Http_200_response(const std::string& filename);
    std::string getResponse() const override;
private:
    std::string m_body{""};
};

class Http_404_response final : public I_http_response
{
public:
    std::string getResponse() const override;
private:
};

class Form_response_factory
{
public:
    static std::string create_response(const http_response_type type, const std::string& content_filename);
};

} //namespace server

#endif // I_HTTP_RESPONSE_H_INCLUDED