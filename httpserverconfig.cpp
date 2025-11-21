#include "httpserverconfig.h"
#include <fstream>
#include <sstream>
#include <iostream>

HttpServerConfig::HttpServerConfig()
{
    port = 8080;          // giá trị mặc định
    documentRoot = "www"; // mặc định
}

HttpServerConfig::~HttpServerConfig()
{
}

// Hàm tách danh sách phần mở rộng "html,htm,jpg"
void HttpServerConfig::parseExtensions(const string& csv)
{
    allowedExtensions.clear();
    string item;
    stringstream ss(csv);

    while (getline(ss, item, ','))
    {
        if (!item.empty())
            allowedExtensions.push_back(item);
    }
}

// Đọc file cấu hình http.conf
bool HttpServerConfig::loadConfig(const string& filename)
{
    ifstream fin(filename);
    if (!fin.is_open())
    {
        cerr << "Khong mo duoc file config: " << filename << endl;
        return false;
    }

    string line;
    string name, value;

    while (getline(fin, line))
    {
        if (readAttribute(line, name, value))
        {
            if (name == "port")
            {
                port = stoi(value);
            }
            else if (name == "document_root")
            {
                documentRoot = value;
            }
            else if (name == "allowed_extension")
            {
                parseExtensions(value);
            }
            else if (name == "timeout")
            {
                this->setTimeOut(stoi(value));
            }
        }
    }

    fin.close();
    return true;
}
