#include "httpserver.h"
#include <iostream>

HttpServer::HttpServer()
    : TCPServer(0)     // ban đầu chưa set port, sẽ set sau khi load config
{
    httpConf = new HttpServerConfig();
    this->conf = httpConf;       // TCPServer sử dụng con trỏ conf chung

    if (loadConfig())
    {
        this->port = httpConf->getPort();
        std::cout << "HTTP Server config loaded. Port = " << this->port << "\n";
    }
    else
    {
        std::cout << "Failed to load http.conf. Use default port 8080\n";
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

// Mỗi slave socket → tạo 1 phiên làm việc
// Skeleton: tạm thời chỉ đóng kết nối ngay (vì chưa code HttpSession)
void HttpServer::startNewSession(TcpSocket slave)
{
    std::cout << "[HttpServer] New connection accepted\n";

    // non-persistent → đóng ngay
    try
    {
        slave.close();
    }
    catch (...)
    {
        std::cerr << "Error when closing slave socket\n";
    }
}
