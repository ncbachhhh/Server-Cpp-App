#include "httpserver.h"
#include "httpsession.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>

// ======================
// Helper: lấy thời gian
// ======================
static string currentDateTime()
{
    time_t now = time(nullptr);
    tm* lt = localtime(&now);
    stringstream ss;
    ss << put_time(lt, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// ======================
// Helper: ghi log khi có kết nối mới
// ======================
static void logConnection(TcpSocket& sock)
{
    ofstream log("server.log", ios::app);    // ghi thêm vào file log
    if (!log.is_open()) return;

    string ip = "-";
    unsigned short port = 0;

    try
    {
        ip = sock.getRemoteAddress();
        port = sock.getRemotePort();
    }
    catch (...) {}

    log << "[" << currentDateTime() << "] "
        << "NEW CONNECTION from " << ip << ":" << port << "\n";
}

// ======================
// HttpServer
// ======================
HttpServer::HttpServer() : TCPServer(0)
{
    httpConf = new HttpServerConfig();
    this->conf = httpConf;  // gán vào config của TCPServer

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

bool HttpServer::loadConfig()
{
    return httpConf->loadConfig("http.conf");
}

// =============================================
// Mỗi slave socket -> tạo 1 phiên HttpSession
// =============================================
void HttpServer::startNewSession(TcpSocket slave)
{
    // Ghi log kết nối và in ra màn hình
    logConnection(slave);

    std::cout << "New connection accepted from "
              << slave.getRemoteAddress()
              << ":" << slave.getRemotePort()
              << std::endl;

    try
    {
        HttpSession session(slave, httpConf);   // tạo phiên mới
        session.handleRequest();                // xử lý request HTTP (GET/HEAD)
    }
    catch (...)
    {
        std::cerr << "Error while processing session\n";
    }
}
