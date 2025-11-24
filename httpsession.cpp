#include "httpsession.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

HttpSession::HttpSession(const TcpSocket& slave, HttpServerConfig* conf)
    : Session(slave, conf)
{
    httpConf = conf;
}

HttpSession::~HttpSession()
{
}

// xử lý yêu cầu
void HttpSession::handleRequest()
{
    try
    {
        char lineBuf[SERVER_CMD_BUF_LEN];

        // nhận và đọc request line
        int r = slave.recvLine(lineBuf, sizeof(lineBuf));

        if (r <= 0) { // nếu k nhận được thì đóng kêt nối
            slave.close();
            return;
        }

        string reqLine(lineBuf);

        if (!parseRequestLine(reqLine))
        {
            vector<char> body;
            string msg = "400 Bad Request";
            body.assign(msg.begin(), msg.end());
            sendResponse(400, "Bad Request", "text/plain", &body, (long)body.size());
            slave.close();
            return;
        }

        // đọc đến dòng trống sau header
        while (true)
        {
            r = slave.recvLine(lineBuf, sizeof(lineBuf));
            if (r <= 0) break;
            string h(lineBuf);
            if (h == "\r\n") break; // dòng trống kết thúc header
        }

        // kiểm tra path tránh path transversal
        if (isBadPath(uri))
        {
            vector<char> body;
            string msg = "403 Forbidden";
            body.assign(msg.begin(), msg.end());
            sendResponse(403, "Forbidden", "text/plain", &body, (long)body.size());
            slave.close();
            return;
        }

        // xử lý theo các method khác nhau
        if (method == "GET") doGET();
        else if (method == "HEAD") doHEAD();
        else doUnsupported();

        slave.close();
    }
    catch (...)
    {
        try
        {
            slave.close();
        } catch (...) {}
    }
}

bool HttpSession::parseRequestLine(const string& line)
{
    // VD: METHOD URI HTTP/1.0\r\n
    string clean = line;

    // xóa \r\n ở cuối dòng
    if (clean.size() >= 2 && clean.substr(clean.size()-2) == "\r\n")
        clean = clean.substr(0, clean.size()-2);

    // tách 3 phần
    stringstream ss(clean);
    ss >> method >> uri >> version;

    if (method.empty() || uri.empty() || version.empty())
        return false;

    // chuyển method sang viết hoa
    for (char& c : method) c = toupper(c);

    // nếu client gửi "/" thì mặc định index.html
    if (uri == "/") uri = "/index.html";

    return true;
}

void HttpSession::doGET()
{
    string path = buildFullPath(uri);

    // check extension
    if (!isAllowedExtension(path))
    {
        vector<char> body;
        string msg = "403 Forbidden";
        body.assign(msg.begin(), msg.end());
        sendResponse(403, "Forbidden", "text/plain", &body, (long)body.size());
        return;
    }

    // đọc file
    vector<char> data;
    if (!readFileBinary(path, data))
    {
        vector<char> body;
        string msg = "404 Not Found";
        body.assign(msg.begin(), msg.end());
        sendResponse(404, "Not Found", "text/plain", &body, (long)body.size());
        return;
    }

    // set header theo file và độ dài file
    string ext = getExtension(path);
    string ctype = getContentType(ext);
    sendResponse(200, "OK", ctype, &data, (long)data.size());
}

void HttpSession::doHEAD() // k gửi body
{
    string path = buildFullPath(uri);

    if (!isAllowedExtension(path))
    {
        sendResponse(403, "Forbidden", "text/plain", nullptr, 0);
        return;
    }

    long fsize = getFileSize(path);
    if (fsize < 0)
    {
        sendResponse(404, "Not Found", "text/plain", nullptr, 0);
        return;
    }

    string ext = getExtension(path);
    string ctype = getContentType(ext);

    sendResponse(200, "OK", ctype, nullptr, fsize); // k gửi body
}

void HttpSession::doUnsupported()
{
    vector<char> body;
    string msg = "405 Method Not Allowed";
    body.assign(msg.begin(), msg.end());
    sendResponse(405, "Method Not Allowed", "text/plain", &body, (long)body.size());
}

void HttpSession::doUnknown(string[], int)
{
    doUnsupported();
}

// ---- helpers ----

bool HttpSession::isBadPath(const string& u)
{
    // filter 1 số ký tự gây path transversal
    if (u.find("..") != string::npos) return true;
    if (u.find("\\") != string::npos) return true;
    if (u.find(":") != string::npos) return true; // tránh scheme lạ
    return false;
}

string HttpSession::buildFullPath(const string& u)
{
    string root = httpConf->getDocumentRoot();
    string rel = u;

    if (!rel.empty() && rel[0] == '/')
        rel = rel.substr(1);

    // ghép root + "/" + uri
    if (!root.empty() && (root.back() == '/' || root.back() == '\\'))
        return root + rel;
    return root + "/" + rel;
}

string HttpSession::getExtension(const string& path) // lấy extention của file
{
    size_t p = path.find_last_of('.');
    if (p == string::npos) return "";
    string ext = path.substr(p + 1);
    for (char& c : ext) c = (char)tolower(c);
    return ext;
}

bool HttpSession::isAllowedExtension(const string& path)
{
    string ext = getExtension(path);
    if (ext.empty()) return false;

    const vector<string>& allow = httpConf->getAllowedExtensions();
    for (string a : allow)
    {
        for (char& c : a) c = (char)tolower(c);
        if (a == ext) return true;
    }
    return false;
}

string HttpSession::getContentType(const string& ext)
{
    if (ext == "html") return "text/html";

    if (ext == "txt") return "text/plain";

    if (ext == "css") return "text/css";

    if (ext == "js") return "application/javascript";

    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";

    if (ext == "png") return "image/png";

    if (ext == "gif") return "image/gif";

    return "application/octet-stream";
}

bool HttpSession::readFileBinary(const string& path, vector<char>& data)
{
    ifstream fin(path, ios::binary);
    if (!fin.is_open()) return false;

    fin.seekg(0, ios::end);
    long sz = (long)fin.tellg();
    fin.seekg(0, ios::beg);

    if (sz < 0) { fin.close(); return false; }

    data.resize(sz);
    if (sz > 0)
        fin.read(&data[0], sz);

    fin.close();
    return true;
}

long HttpSession::getFileSize(const string& path)
{
    ifstream fin(path, ios::binary);
    if (!fin.is_open()) return -1;
    fin.seekg(0, ios::end);
    long sz = (long)fin.tellg();
    fin.close();
    return sz;
}

void HttpSession::sendAll(const char* data, int len)
{
    int sent = 0;
    while (sent < len)
    {
        int r = slave.send(data + sent, len - sent);
        if (r <= 0) break;
        sent += r;
    }
}

void HttpSession::sendResponse(int code, const string& reason,
                               const string& contentType,
                               const vector<char>* body, long contentLen)
{
    // status line + headers
    stringstream ss;
    ss << "HTTP/1.0 " << code << " " << reason << "\r\n";
    ss << "Content-Type: " << contentType << "\r\n";
    ss << "Content-Length: " << contentLen << "\r\n";
    ss << "Connection: close\r\n\r\n";

    string header = ss.str();
    sendAll(header.c_str(), (int)header.size());

    // body nếu có
    if (body != nullptr && !body->empty())
        sendAll(&(*body)[0], (int)body->size());
}

