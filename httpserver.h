#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "tcpserver.h"
#include "httpserverconfig.h"
#include "tcpsocket.h"

class HttpServer : public TCPServer
{
private:
    HttpServerConfig* httpConf;   // config cho server

public:
    HttpServer();
    virtual ~HttpServer();

    bool loadConfig();            // đọc http.conf

protected:
    // override từ TCPServer
    virtual void startNewSession(TcpSocket slave) override;
};

#endif
