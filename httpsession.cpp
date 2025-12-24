#include "httpsession.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <iomanip>

using namespace std;

// lấy thời gian
static string currentDateTime()
{
    time_t now = time(nullptr);                     // Lấy thời gian hiện tại
    tm* lt = localtime(&now);                       // Chuyển sang local time
    stringstream ss;                               
    ss << put_time(lt, "%Y-%m-%d %H:%M:%S");        // Format: YYYY-MM-DD HH:MM:SS
    return ss.str();                                // Trả về chuỗi thời gian
}

// ghi access log
static void logAccess(const string& ip,
                      const string& method,
                      const string& uri,
                      int code,
                      long len)
{
    ofstream log("logs/access.log", ios::app);     // Mở file log ở chế độ append
    if (!log.is_open()) return;                     // Nếu không mở được thì thoát

    // Ghi log theo format: [thời gian] IP "METHOD URI HTTP/1.0" status_code content_length
    log << "[" << currentDateTime() << "] "
        << ip << " \"" << method << " " << uri << " HTTP/1.0\" "
        << code << " " << len << "\n";
}

HttpSession::HttpSession(const TcpSocket& slave, HttpServerConfig* conf)
    : Session(slave, conf)
{
    httpConf = conf;        // Lưu con trỏ cấu hình HTTP
}

HttpSession::~HttpSession()
{
}

// Đọc 1 request, xử lý xong là đóng kết nối
void HttpSession::handleRequest()
{
    try
    {
        char lineBuf[SERVER_CMD_BUF_LEN];           // Buffer để đọc từng dòng

        // nhận và đọc request line
        int r = slave.recvLine(lineBuf, sizeof(lineBuf));   // Đọc dòng đầu tiên (request line)
        if (r <= 0)                                 // Nếu không đọc được dữ liệu
        {
            slave.close();                          // Đóng kết nối
            return;
        }

        string reqLine(lineBuf);                    // Chuyển buffer thành string
        if (!parseRequestLine(reqLine))             // Parse request line (METHOD URI VERSION)
        {
            vector<char> body;
            string msg = "400 Bad Request";
            body.assign(msg.begin(), msg.end());
            sendResponse(400, "Bad Request", "text/plain", &body, (long)body.size());   // Gửi lỗi 400
            slave.close();
            return;
        }

        // đọc và bỏ qua headers cho tới dòng trống
        while (true)
        {
            r = slave.recvLine(lineBuf, sizeof(lineBuf));   // Đọc từng dòng header
            if (r <= 0) break;                              // Nếu không đọc được thì dừng
            string h(lineBuf);
            if (h == "\r\n") break;                         // Dòng trống => kết thúc header
        }

        // kiểm tra path tránh injection
        if (isBadPath(uri))                         // Kiểm tra path có ký tự nguy hiểm không
        {
            vector<char> body;
            string msg = "403 Forbidden";
            body.assign(msg.begin(), msg.end());
            sendResponse(403, "Forbidden", "text/plain", &body, (long)body.size());     // Gửi lỗi 403
            slave.close();
            return;
        }

        // xử lý theo method
        if (method == "GET")                        // Nếu là GET request
            doGET();
        else if (method == "HEAD")                  // Nếu là HEAD request
            doHEAD();
        else                                        // Các method khác không hỗ trợ
            doUnsupported();

        // xử lý xong là đóng kết nối
        slave.close();                              // Đóng kết nối (HTTP/1.0 không keep-alive)
    }
    catch (...)
    {
        try { slave.close(); } catch (...) {}       // Đảm bảo đóng kết nối khi có exception
    }
}

bool HttpSession::parseRequestLine(const string& line)
{
    // line dạng: METHOD URI HTTP/1.0\r\n
    string clean = line;

    // bỏ \r\n ở cuối nếu có
    if (clean.size() >= 2 && clean.substr(clean.size() - 2) == "\r\n")
        clean = clean.substr(0, clean.size() - 2);      // Cắt bỏ 2 ký tự cuối (\r\n)

    stringstream ss(clean);                             // Tạo stream để parse
    ss >> method >> uri >> version;                     // Tách thành 3 phần: method, uri, version

    if (method.empty() || uri.empty() || version.empty())   // Kiểm tra các phần có rỗng không
        return false;

    // chuyển method về chữ hoa
    for (char& c : method) c = (char)toupper(c);        // GET, POST, HEAD,...

    // nếu chỉ "/" thì mặc định index.html
    if (uri == "/")
        uri = "/index.html";                            // Chuyển / thành /index.html

    return true;
}

void HttpSession::doGET()
{
    string path = buildFullPath(uri);               // Tạo đường dẫn đầy đủ từ URI

    // kiểm tra extension hợp lệ
    if (!isAllowedExtension(path))                  // Kiểm tra extension có được phép không
    {
        vector<char> body;
        string msg = "403 Forbidden";
        body.assign(msg.begin(), msg.end());
        sendResponse(403, "Forbidden", "text/plain", &body, (long)body.size());
        return;
    }

    vector<char> data;                              // Vector để chứa dữ liệu file
    if (!readFileBinary(path, data))                // Đọc file dưới dạng binary
    {
        vector<char> body;
        string msg = "404 Not Found";
        body.assign(msg.begin(), msg.end());
        sendResponse(404, "Not Found", "text/plain", &body, (long)body.size());     // File không tồn tại
        return;
    }

    string ext = getExtension(path);                // Lấy extension từ path
    string ctype = getContentType(ext);             // Xác định Content-Type
    sendResponse(200, "OK", ctype, &data, (long)data.size());   // Gửi response thành công
}

void HttpSession::doHEAD()
{
    string path = buildFullPath(uri);               // Tạo đường dẫn đầy đủ

    if (!isAllowedExtension(path))                  // Kiểm tra extension
    {
        sendResponse(403, "Forbidden", "text/plain", nullptr, 0);
        return;
    }

    long fsize = getFileSize(path);                 // Lấy kích thước file
    if (fsize < 0)                                  // File không tồn tại hoặc lỗi
    {
        sendResponse(404, "Not Found", "text/plain", nullptr, 0);
        return;
    }

    string ext = getExtension(path);                // Lấy extension
    string ctype = getContentType(ext);             // Xác định Content-Type

    // HEAD: chỉ header, không body
    sendResponse(200, "OK", ctype, nullptr, fsize); // Gửi header với Content-Length, không gửi body
}

void HttpSession::doUnsupported()
{
    vector<char> body;
    string msg = "405 Method Not Allowed";
    body.assign(msg.begin(), msg.end());
    sendResponse(405, "Method Not Allowed", "text/plain", &body, (long)body.size());  // Gửi lỗi 405
}

void HttpSession::doUnknown(string[], int)
{
    doUnsupported();        // Chuyển sang xử lý method không hỗ trợ
}


bool HttpSession::isBadPath(const string& u)
{
    // chặn ../, \, : để tránh traversal/đường dẫn bất thường
    if (u.find("..") != string::npos) return true;      // Chặn path traversal (../../../etc/passwd)
    if (u.find("\\") != string::npos) return true;      // Chặn backslash (Windows path)
    if (u.find(":") != string::npos) return true;       // Chặn absolute path (C:/...)
    return false;
}

string HttpSession::buildFullPath(const string& u)
{
    string root = httpConf->getDocumentRoot();      // Lấy document root từ config
    string rel = u;                                 // URI tương đối

    if (!rel.empty() && rel[0] == '/')              // Nếu bắt đầu bằng /
        rel = rel.substr(1);                        // Bỏ / ở đầu

    if (!root.empty() && (root.back() == '/' || root.back() == '\\'))   // Nếu root có / hoặc \ ở cuối
        return root + rel;                          // Ghép trực tiếp

    return root + "/" + rel;                        // Ghép với dấu /
}

string HttpSession::getExtension(const string& path)
{
    size_t p = path.find_last_of('.');              // Tìm dấu . cuối cùng
    if (p == string::npos) return "";               // Không có extension
    string ext = path.substr(p + 1);                // Lấy phần sau dấu .
    for (char& c : ext) c = (char)tolower(c);       // Chuyển về chữ thường
    return ext;
}

bool HttpSession::isAllowedExtension(const string& path)
{
    string ext = getExtension(path);                // Lấy extension từ path
    if (ext.empty()) return false;                  // Không có extension => không cho phép

    const vector<string>& allow = httpConf->getAllowedExtensions();     // Lấy danh sách extension cho phép
    for (string a : allow)                          // Duyệt qua từng extension
    {
        for (char& c : a) c = (char)tolower(c);     // Chuyển về chữ thường
        if (a == ext) return true;                  // Tìm thấy => cho phép
    }
    return false;                                   // Không tìm thấy => cấm
}

string HttpSession::getContentType(const string& ext)
{
    // Map extension sang MIME type
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "txt") return "text/plain";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    return "application/octet-stream";              // Mặc định cho file binary
}

bool HttpSession::readFileBinary(const string& path, vector<char>& data)
{
    ifstream fin(path, ios::binary);                // Mở file ở chế độ binary
    if (!fin.is_open()) return false;               // Không mở được => lỗi

    fin.seekg(0, ios::end);                         // Di chuyển con trỏ đến cuối file
    long sz = (long)fin.tellg();                    // Lấy vị trí = kích thước file
    fin.seekg(0, ios::beg);                         // Quay về đầu file

    if (sz < 0)                                     // Kích thước không hợp lệ
    {
        fin.close();
        return false;
    }

    data.resize(sz);                                // Cấp phát bộ nhớ cho vector
    if (sz > 0)                                     // Nếu file không rỗng
        fin.read(&data[0], sz);                     // Đọc toàn bộ dữ liệu vào vector

    fin.close();                                    // Đóng file
    return true;
}

long HttpSession::getFileSize(const string& path)
{
    ifstream fin(path, ios::binary);                // Mở file ở chế độ binary
    if (!fin.is_open()) return -1;                  // Không mở được => trả về -1
    fin.seekg(0, ios::end);                         // Di chuyển đến cuối file
    long sz = (long)fin.tellg();                    // Lấy vị trí = kích thước
    fin.close();                                    // Đóng file
    return sz;                                      // Trả về kích thước
}

// gửi kết quả toàn bộ
void HttpSession::sendAll(const char* data, int len)
{
    int sent = 0;                                   // Số byte đã gửi
    while (sent < len)                              // Lặp cho đến khi gửi hết
    {
        int r = slave.send(data + sent, len - sent);    // Gửi phần còn lại
        if (r <= 0) break;                          // Lỗi hoặc connection đóng => dừng
        sent += r;                                  // Cộng thêm số byte đã gửi
    }
}

void HttpSession::sendResponse(int code, const string& reason,
                               const string& contentType,
                               const vector<char>* body, long contentLen)
{
    // status line + headers
    stringstream ss;
    ss << "HTTP/1.0 " << code << " " << reason << "\r\n";      // Status line
    ss << "Server: SimpleCppHttpServer\r\n";                   // Server header
    ss << "Connection: close\r\n";                             // Không keep-alive
    ss << "Content-Type: " << contentType << "\r\n";           // MIME type
    ss << "Content-Length: " << contentLen << "\r\n\r\n";      // Kích thước body + dòng trống

    string header = ss.str();                       // Chuyển sang string
    sendAll(header.c_str(), (int)header.size());   // Gửi header

    // body nếu có (GET hoặc lỗi)
    if (body != nullptr && !body->empty())          // Nếu có body
        sendAll(&(*body)[0], (int)body->size());   // Gửi body

    // ghi log access sau khi gửi response
    string ip = "-";
    try
    {
        ip = slave.getRemoteAddress();              // Lấy IP của client
    }
    catch (...) {}

    logAccess(ip, method, uri, code, contentLen);  // Ghi vào access log
}
