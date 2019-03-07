#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <map>
#include <unordered_map>
#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <exception>
#include <stdexcept>

#define BUFFER_SIZE 1024

class Request{
 public:
  std::vector<char> request;
  std::string request_line;
  std::string method;
  std::string hostname;
  std::string IP;
  size_t header_length;
  size_t content_length;
  size_t total_length;

  //constructor
  Request(): header_length(0),content_length(0),total_length(0){
    request.resize(BUFFER_SIZE,0);
  }
  //copy constructor
  Request(const Request &rhs){
    request = rhs.request;
    request_line=rhs.request_line;
    method=rhs.method;
    hostname=rhs.hostname;
    IP=rhs.IP;
    header_length=rhs.header_length;
    content_length=rhs.content_length;
    total_length=rhs.total_length;
  }
  //assignment operator
  Request& operator=(const Request &rhs){
    if(this!= &rhs){
      request = rhs.request;
      request_line=rhs.request_line;
      method=rhs.method;
      hostname=rhs.hostname;
      IP=rhs.IP;
      header_length=rhs.header_length;
      content_length=rhs.content_length;
      total_length=rhs.total_length;
    }
    return *this;
  }
  ~Request(){}

  bool headerEnd(){
    std::string req(request.begin(),request.end());
    //find the end
    size_t end=req.find("\r\n\r\n");
    if(end!=std::string::npos){
      header_length=end+4;
      return true;
    }
    return false;
  }
 
  void getcontent(){
    std::string req(request.begin(),request.end());
    int end=req.find("\r\n");
    request_line=req.substr(0,end);
    if(request_line.find("HTTP/1.1")==std::string::npos){
       throw std::exception();
    }
  }

  void getmethod(){
    std::string req(request.begin(),request.end());
    int end=req.find(" ");
    method=req.substr(0,end);
  }

  void gethostname(){
    std::string req(request.begin(),request.end());
    int title=req.find("Host: ");
    int end=req.find("\r\n",title);
    hostname=req.substr(title+6,end-title-6);
    if(method=="CONNECT"){
      int colon=hostname.find_last_of(":");
      hostname=hostname.substr(0,colon);
    }
    //get IP from hostname
    struct hostent *host=gethostbyname(hostname.c_str());
    if(host==NULL){
      throw std::exception();
    }
    struct sockaddr_in sock_addr;
    char buffer[20];
    sock_addr.sin_addr =*((struct in_addr*) host->h_addr_list[0]);
    inet_ntop(AF_INET, &sock_addr.sin_addr,buffer,sizeof(buffer));
    IP=buffer;
  }

  void getContentlength(){
    std::string req(request.begin(),request.end());
    int title=req.find("Content-Length: ");
    int end=req.find("\r\n",title);
    std::string length_str=req.substr(title+16,end-title-16);
    content_length=stoi(length_str);
  }

};

#endif
  
