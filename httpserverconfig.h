#ifndef HTTP_SERVER_CONFIG_H
#define HTTP_SERVER_CONFIG_H

#include "serverconfig.h"
#include <string>
#include <vector>

using namespace std;

class HttpServerConfig : public ServerConfig
{
private:
    string documentRoot;                  // thư mục gốc chứa tài nguyên
    vector<string> allowedExtensions;     // danh sách phần mở rộng hợp lệ
    unsigned short port;                  // cổng server

public:
    HttpServerConfig();
    virtual ~HttpServerConfig();

    bool loadAccountsFromFile(const string& filename) override { return true; }

    // File cấu hình: http.conf
    bool loadConfig(const string& filename);

    // Getter
    const string& getDocumentRoot() const { return documentRoot; }
    const vector<string>& getAllowedExtensions() const { return allowedExtensions; }
    unsigned short getPort() const { return port; }

private:
    void parseExtensions(const string& csv);
};

#endif
