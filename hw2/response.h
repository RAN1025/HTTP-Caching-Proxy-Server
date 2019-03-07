#ifndef __RESPONSE_H__
#define __RESPONSE_H__

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

class Response{
 public:
  std::vector<char> response;
  std::string status;
  std::string status_code;
  std::string date;//time when server gives response
  std::string last_modified;
  std::string cache_control;//cache control tag
  std::string expires;
  std::string etag;
  size_t content_length;
  size_t header_length;
  size_t total_length;
  bool ischunk;
  time_t start_store_time;
 
  //constructor
 Response(): content_length(0),header_length(0),total_length(0),ischunk(false),start_store_time(0){
    response.resize(BUFFER_SIZE,0);
  }
  //copy constructor
  Response(const Response &rhs){
    response=rhs.response;
    status=rhs.status;
    status_code=rhs.status_code;
    date=rhs.date;
    last_modified=rhs.last_modified;
    cache_control=rhs.cache_control;
    expires=rhs.expires;
    etag=rhs.etag;
    content_length=rhs.content_length;
    header_length=rhs.header_length;
    total_length=rhs.total_length;
    ischunk=rhs.ischunk;
    start_store_time=rhs.start_store_time;
  }  
  //assignment operator                                          
  Response& operator=(const Response &rhs){
    if(this!= &rhs){
      response=rhs.response;
      status=rhs.status;
      status_code=rhs.status_code;
      date=rhs.date;
      last_modified=rhs.last_modified;
      cache_control=rhs.cache_control;
      expires=rhs.expires;
      etag=rhs.etag;
      content_length=rhs.content_length;
      header_length=rhs.header_length;
      total_length=rhs.total_length;
      ischunk=rhs.ischunk;
      start_store_time=rhs.start_store_time;
    }
    return *this;
  }
  ~Response(){}

  bool headerEnd(){
    std::string res(response.begin(),response.end());
    //find the end
    size_t end=res.find("\r\n\r\n");
    if(end!=std::string::npos){
      header_length=end+4;
      return true;
    }
    return false;
  }
  
  std::string parseToTag(std::string response, int pos){
    std::string remain;
    int line_end;
    std::string temp;
    int pos_blank;
    remain=response.substr(pos);
    line_end=remain.find("\r\n");
    temp=remain.substr(0, line_end);
    pos_blank=temp.find(" ");
    return temp.substr(pos_blank+1);
  }

  void parseResponse(){
    std::string res(response.begin(),response.end());
    int firstlinepos=res.find("\r\n");
    status=res.substr(0, firstlinepos);
    if(status.find("HTTP/1.1")==std::string::npos){
      throw std::exception();
    }
    int after=status.find_last_of(" ");
    int before=status.find_last_of(" ",after-1);
    status_code=status.substr(before+1,after-before-1);
    size_t date_pos=res.find("Date");
    if(date_pos!=std::string::npos){
      date=parseToTag(res, date_pos);
    }
    size_t last_modified_pos=res.find("Last-Modified");
    if(last_modified_pos!=std::string::npos){
      last_modified=parseToTag(res, last_modified_pos);
    }
    size_t cache_control_pos=res.find("Cache-Control");
    if(cache_control_pos!=std::string::npos){
      cache_control=parseToTag(res, cache_control_pos);
    }
    size_t expires_pos=res.find("Expires");
    if(expires_pos!=std::string::npos){
      expires=parseToTag(res, expires_pos);
    }
    size_t etag_pos=res.find("ETag");
    if(etag_pos!=std::string::npos){
      etag=parseToTag(res, etag_pos);
    }
  }

  void getContentlength(){
    std::string res(response.begin(),response.end());
    size_t title=res.find("Content-Length: ");
    if(title==std::string::npos){
      ischunk=true;
    }
    else{
      int end=res.find("\r\n",title);
      std::string length_str=res.substr(title+16,end-title-16);
      content_length=stoi(length_str);
    }
  }

};

#endif
