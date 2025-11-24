#include "httpserverconfig.h"
#include <fstream>
#include <sstream>
#include <iostream>

HttpServerConfig::HttpServerConfig()
{
    // mặc định trước khi đọc được file config
    port = 8080;
    documentRoot = "www";
}

HttpServerConfig::~HttpServerConfig()
{
}

// parse danh sách các extention từ file
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

// đọc file http.conf
bool HttpServerConfig::loadConfig(const string& filename)
{
    ifstream fin(filename);
    if (!fin.is_open())
    {
        cerr << "Can not open file config: " << filename << endl;
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
