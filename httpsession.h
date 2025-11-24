#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include "session.h"
#include "httpserverconfig.h"
#include <string>
#include <vector>

using namespace std;

class HttpSession : public Session
{
private:
    HttpServerConfig* httpConf;     // trỏ tới cấu hình HTTP
    string method, uri, version;    // các thành phần của request line

public:
    HttpSession(const TcpSocket& slave, HttpServerConfig* conf);
    virtual ~HttpSession();

    void handleRequest();

    // override từ Session, dùng cho command không hợp lệ
    virtual void doUnknown(string cmd_argv[], int cmd_argc) override;

private:
    // Tách METHOD, URI, VERSION từ dòng đầu tiên của request
    bool parseRequestLine(const string& line);

    void doGET();
    void doHEAD();
    void doUnsupported();

    // các hàm helpers

    string buildFullPath(const string& uri);

    bool isAllowedExtension(const string& path);

    string getExtension(const string& path);

    string getContentType(const string& ext);

    // tránh lỗ hổng path transversal
    bool isBadPath(const string& uri);

    // đọc dữ liệu file
    bool readFileBinary(const string& path, vector<char>& data);

    // Lấy kích thước file
    long getFileSize(const string& path);

    // Gửi toàn bộ response HTTP (status + header + body)
    void sendResponse(int code,const string& reason, const string& contentType, const vector<char>* body, long contentLen);

    // Hàm gửi toàn bộ buffer qua socket
    void sendAll(const char* data, int len);
};

#endif
