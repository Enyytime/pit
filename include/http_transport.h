#ifndef HTTP_TRANSPORT_H
#define HTTP_TRANSPORT_H
int http_fetch(const char* remote_name, const char* url, const char* branch);
int http_push(const char* url, const char* branch);
#endif