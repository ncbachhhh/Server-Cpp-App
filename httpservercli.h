#ifndef HTTP_SERVER_CLI_H
#define HTTP_SERVER_CLI_H

#include "cli.h"
#include "httpserver.h"

class HttpServerCLI : public CmdLineInterface
{
private:
    HttpServer* server;

public:
    HttpServerCLI();
    virtual ~HttpServerCLI();

protected:
    virtual void initCmd() override;

private:
    void doStart(string cmd_argv[], int cmd_argc);
    void doStop(string cmd_argv[], int cmd_argc);
    void doRestart(string cmd_argv[], int cmd_argc);
    void doStatus(string cmd_argv[], int cmd_argc);
    void doHelp(string cmd_argv[], int cmd_argc);
};

#endif

