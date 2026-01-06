#include "httpservercli.h"
#include <iostream>

HttpServerCLI::HttpServerCLI() : CmdLineInterface("Server> ")
{
    server = new HttpServer();
    initCmd();
}

HttpServerCLI::~HttpServerCLI()
{
    if (server) delete server;
}

void HttpServerCLI::initCmd()
{
    addCmd("start",   CLI_CAST(&HttpServerCLI::doStart));
    addCmd("stop",    CLI_CAST(&HttpServerCLI::doStop));
    addCmd("restart", CLI_CAST(&HttpServerCLI::doRestart));
    addCmd("status",  CLI_CAST(&HttpServerCLI::doStatus));
    addCmd("help",    CLI_CAST(&HttpServerCLI::doHelp));
}

// Start
void HttpServerCLI::doStart(string cmd_argv[], int cmd_argc)
{
    if (server->isRunning())
    {
        cout << "HTTP Server is running\n";
        return;
    }

    if (server->start())
        cout << "HTTP Server started on port " << server->getPort() << "\n";
    else
        cout << "HTTP Server failed to start\n";
}

// Stop
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

// Restart
void HttpServerCLI::doRestart(string cmd_argv[], int cmd_argc)
{
    if (server->restart())
        cout << "HTTP Server restarted\n";
    else
        cout << "HTTP Server failed to restart\n";
}

// Status
void HttpServerCLI::doStatus(string cmd_argv[], int cmd_argc)
{
    if (server->isRunning())
        cout << "HTTP Server is running\n";
    else
        cout << "HTTP Server is not running\n";
}

// Help
void HttpServerCLI::doHelp(string cmd_argv[], int cmd_argc)
{
    cout << "Cac lenh ho tro:\n";
    cout << "  start    : Start server\n";
    cout << "  stop     : Stop server\n";
    cout << "  restart  : Restart server\n";
    cout << "  status   : Status server\n";
    cout << "  help     : Show command\n";
    cout << "  quit     : Exit the program\n";
}

