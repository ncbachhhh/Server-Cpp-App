#include "httpserver.h"
#include "httpsession.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>

// Lấy thời gian
static string currentDateTime()
{
    time_t now = time(nullptr);                     // Lấy thời gian hiện tại (epoch)
    tm* lt = localtime(&now);                       // Chuyển sang local time
    stringstream ss;
    ss << put_time(lt, "%Y-%m-%d %H:%M:%S");       // Format: YYYY-MM-DD HH:MM:SS
    return ss.str();                                // Trả về chuỗi thời gian
}

// Lấy log khi có kết nối
static void logConnection(TcpSocket& sock)
{
    ofstream log("./logs/server.log", ios::app);     // Mở file log ở chế độ append
    if (!log.is_open()) return;                     // Nếu không mở được thì thoát

    string ip = "-";                                // IP mặc định nếu không lấy được
    unsigned short port = 8080;                        // Port mặc định

    try
    {
        ip = sock.getRemoteAddress();               // Lấy địa chỉ IP của client
        port = sock.getRemotePort();                // Lấy port của client
    }
    catch (...) {}                                  // Bỏ qua lỗi nếu không lấy được

    // Ghi log theo format: [thời gian] NEW CONNECTION from IP:PORT
    log << "[" << currentDateTime() << "] "
        << "NEW CONNECTION from " << ip << ":" << port << "\n";
}


HttpServer::HttpServer() : TCPServer(0)
{
    httpConf = new HttpServerConfig();              // Tạo đối tượng config cho HTTP server
    this->conf = httpConf;                          // Gán vào config của TCPServer (class cha)

    if (loadConfig())                               // Thử load file config
    {
        this->port = httpConf->getPort();           // Lấy port từ config
        std::cout << "HTTP Server config loaded. Port = " << this->port << "\n";
    }
    else                                            // Nếu load config thất bại
    {
        std::cout << "Failed to load config. Use default port 8080\n";
        this->port = 8080;                          // Dùng port mặc định
    }
}

HttpServer::~HttpServer()
{
    if (httpConf)                                   // Kiểm tra con trỏ hợp lệ
        delete httpConf;                            // Giải phóng bộ nhớ config
}

bool HttpServer::loadConfig()
{
    return httpConf->loadConfig("http.conf");       // Đọc file http.conf và parse cấu hình
}

void HttpServer::startNewSession(TcpSocket slave)
{
    // Ghi log kết nối và in ra màn hình
    logConnection(slave);                           // Ghi vào file logs/server.log

    std::cout << "New connection accepted from "    // In thông báo ra console
              << slave.getRemoteAddress()           // Địa chỉ IP client
              << ":" << slave.getRemotePort()       // Port của client
              << std::endl;

    try
    {
        HttpSession session(slave, httpConf);       // Tạo phiên HTTP mới cho kết nối này
        session.handleRequest();                    // Xử lý HTTP request
    }
    catch (...)                                     // Bắt mọi exception
    {
        std::cerr << "Error while processing session\n";   // In lỗi ra console
    }
}
