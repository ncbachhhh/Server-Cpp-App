#include "httpservercli.h"
#include <iostream>

HttpServerCLI::HttpServerCLI() : CmdLineInterface("http-server> ")
{
    server = new HttpServer();
    initCmd();
}

HttpServerCLI::~HttpServerCLI()
{
    if (server) delete server;
}

// Đăng ký các lệnh CLI
void HttpServerCLI::initCmd()
{
    addCmd("start",   CLI_CAST(&HttpServerCLI::doStart));
    addCmd("stop",    CLI_CAST(&HttpServerCLI::doStop));
    addCmd("restart", CLI_CAST(&HttpServerCLI::doRestart));
    addCmd("status",  CLI_CAST(&HttpServerCLI::doStatus));
    addCmd("help",    CLI_CAST(&HttpServerCLI::doHelp));
}

// Lệnh start
void HttpServerCLI::doStart(string cmd_argv[], int cmd_argc)
{
    if (server->isRunning())
    {
        cout << "HTTP Server is running\n";
        return;
    }

    if (server->start())
        cout << "HTTP Server started\n";
    else
        cout << "HTTP Server failed to start\n";
}

// Lệnh stop
void HttpServerCLI::doStop(string cmd_argv[], int cmd_argc)
{
    if (!server->isRunning())
    {
        cout << "HTTP Server is not running\n";
        return;
    }

    server->stop();
    cout << "HTTP Server stopped\n";
}

// Lệnh restart
void HttpServerCLI::doRestart(string cmd_argv[], int cmd_argc)
{
    if (server->restart())
        cout << "HTTP Server restarted\n";
    else
        cout << "HTTP Server failed to restart\n";
}

// Lệnh status
void HttpServerCLI::doStatus(string cmd_argv[], int cmd_argc)
{
    if (server->isRunning())
        cout << "HTTP Server is running\n";
    else
        cout << "HTTP Server is not running\n";
}

// Lệnh help
void HttpServerCLI::doHelp(string cmd_argv[], int cmd_argc)
{
    cout << "Cac lenh ho tro:\n";
    cout << "  start    : Bat server\n";
    cout << "  stop     : Tat server\n";
    cout << "  restart  : Khoi dong lai server\n";
    cout << "  status   : Xem trang thai server\n";
    cout << "  help     : Tro giup\n";
    cout << "  quit     : Thoat chuong trinh\n";
}

