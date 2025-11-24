#include "httpserver.h"
#include "httpsession.h"
#include <iostream>

HttpServer::HttpServer():TCPServer(0)     // set tạm port là 0, load config xong thì set lại
{
    httpConf = new HttpServerConfig();
    this->conf = httpConf;

    if (loadConfig())
    {
        this->port = httpConf->getPort();
        std::cout << "HTTP Server config loaded. Port = " << this->port << "\n";
    }
    else
    {
        std::cout << "Failed to load config. Use default port 8080\n";
        this->port = 8080;
    }
}

HttpServer::~HttpServer()
{
    if (httpConf)
        delete httpConf;
}

// Đọc file http.conf
bool HttpServer::loadConfig()
{
    return httpConf->loadConfig("http.conf");
}

// Mỗi slave socket tạo 1 phiên làm việc
void HttpServer::startNewSession(TcpSocket slave)
{
    std::cout << "New connection accepted from " << slave.getRemoteAddress() << ":" << slave.getRemotePort() << endl;

    try
    {
        HttpSession session(slave, httpConf); // tạo session mới
        session.handleRequest(); // xử lý yêu cầu
    }
    catch (...)
    {
        std::cerr << "Error when closing slave socket\n";
    }
}
