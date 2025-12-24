#include "httpserverconfig.h"
#include <fstream>
#include <sstream>
#include <iostream>

HttpServerConfig::HttpServerConfig()
{
    // mặc định trước khi đọc được file config
    port = 8080;              // Cổng mặc định của HTTP server
    documentRoot = "www";     // Thư mục gốc chứa các file web
}

HttpServerConfig::~HttpServerConfig()
{
}

// parse danh sách các extention từ file
void HttpServerConfig::parseExtensions(const string& csv)
{
    allowedExtensions.clear();      // Xóa danh sách cũ
    string item;                    // Biến lưu từng extension
    stringstream ss(csv);           // Tạo stream từ chuỗi CSV

    // Đọc từng extension phân cách bởi dấu phẩy
    while (getline(ss, item, ','))
    {
        if (!item.empty())          // Kiểm tra không rỗng
            allowedExtensions.push_back(item);  // Thêm vào danh sách
    }
}

// đọc file http.conf
bool HttpServerConfig::loadConfig(const string& filename)
{
    ifstream fin(filename);         // Mở file cấu hình
    if (!fin.is_open())             // Kiểm tra mở file thành công
    {
        cerr << "Can not open file config: " << filename << endl;
        return false;               // Trả về false nếu không mở được
    }

    string line;                    // Biến lưu từng dòng đọc được
    string name, value;             // Biến lưu tên thuộc tính và giá trị

    // Đọc từng dòng trong file config
    while (getline(fin, line))
    {
        // Parse dòng thành cặp name-value
        if (readAttribute(line, name, value))
        {
            if (name == "port")     // Nếu là thuộc tính port
            {
                port = stoi(value); // Chuyển chuỗi thành số nguyên
            }
            else if (name == "document_root")   // Nếu là thư mục gốc
            {
                documentRoot = value;           // Gán giá trị
            }
            else if (name == "allowed_extension")   // Nếu là danh sách extension
            {
                parseExtensions(value);             // Parse chuỗi CSV
            }
            else if (name == "timeout")         // Nếu là timeout
            {
                this->setTimeOut(stoi(value));  // Chuyển thành số và set timeout
            }
        }
    }

    fin.close();                    // Đóng file
    return true;                    
}
