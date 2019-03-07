#ifndef __PROXYSERVER_H__
#define __PROXYSERVER_H__

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
#include <time.h>
#include <ctime>
#include <thread>
#include <mutex>
#include <exception>
#include <stdexcept>
#include "request.h"
#include "response.h"
#include "cache.h"

class Proxy{
  //proxy port
  std::string port;
  const char* hostname;
  
 public:
  //proxy server
  int proxy_fd;
  //accpet client
  int client_fd;
  //connect to web server
  int webserver_fd;

  //constructor
 Proxy(std::string port_num):port(port_num),hostname(NULL),proxy_fd(0),client_fd(0),webserver_fd(0){}
  //copy constructor
  Proxy(const Proxy &rhs){
    port = rhs.port;
    hostname = rhs.hostname;
    proxy_fd = rhs.proxy_fd;
    client_fd = rhs.client_fd;
    webserver_fd = rhs.webserver_fd;
  }
  //assignment operator
  Proxy& operator=(const Proxy &rhs){
    if(this!= &rhs){
      port = rhs.port;
      hostname = rhs.hostname;
      proxy_fd = rhs.proxy_fd;
      client_fd = rhs.client_fd;
      webserver_fd = rhs.webserver_fd;
    }
    return *this;
  }
  ~Proxy(){}

  //create the 400,404,502 response
  std::string giveResponse(Request& request,std::string code,std::string reason){
    std::string give="HTTP/1.1 ";
    std::string body="<html><body>"+code+" "+reason+"</body></html>";
    give=give+code;
    give=give+reason;
    give=give+"\r\nContent-Length: ";
    std::string length=std::to_string(body.size());
    give=give+length;
    give=give+"\r\n\r\n";
    give=give+body;
    return give;
  }

  //initialize the proxy, create socket between proxy and web browser
  struct addrinfo * connectWebBrowser(){
    //BEGIN_REF - TCP Example by by Brian Rogers, updated by Rabih Younes in ECE650                  
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;
    status = getaddrinfo(hostname, port.c_str(), &host_info, &host_info_list);
    if (status != 0) {
      std::cerr << "Error: cannot get address info for host" << std::endl;
      std::cerr << "  (" << hostname << "," << port.c_str() << ")" << std::endl;
      exit(EXIT_FAILURE);
    }
    proxy_fd = socket(host_info_list->ai_family,
		      host_info_list->ai_socktype,
		      host_info_list->ai_protocol);
    if (proxy_fd == -1) {
      std::cerr << "Error: cannot create socket" << std::endl;
      std::cerr << "  (" << hostname << "," << port.c_str() << ")" << std::endl;
      exit(EXIT_FAILURE);
    }
    int yes = 1;
    status = setsockopt(proxy_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(proxy_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      std::cerr << "Error: cannot bind socket" << std::endl;
      std::cerr << "  (" << hostname << "," << port.c_str() << ")" << std::endl;
      exit(EXIT_FAILURE);
    }
    status = listen(proxy_fd, 100);
    if (status == -1) {
      std::cerr << "Error: cannot listen on socket" << std::endl;
      std::cerr << "  (" << hostname << "," << port.c_str() << ")" << std::endl;
      exit(EXIT_FAILURE);
    }
    //END_REF                                                                                        
    return host_info_list;
  }
  
  //receive request helper function
  void receiveRequest(Request &request){
    size_t receive_length=0;
    while(1){
      size_t recvbyte=recv(client_fd, &request.request.data()[receive_length],BUFFER_SIZE,0);
      receive_length=receive_length+recvbyte;
      if(recvbyte==0){
	break;
      }
      //receive the header
      if(request.headerEnd()==true){
	try{
	request.getcontent();
	request.getmethod();
	request.gethostname();
	}
      	catch(std::exception &e){
	  //400
	  std::string badrequest=giveResponse(request,"400","Bad Request");
     	  //send badrequest to browser
	  send(client_fd,badrequest.c_str(),badrequest.length(),0);
	  throw std::exception();
	}
	break;
      }
      else{
	//resize and continue receive
	try{
	  request.request.resize(receive_length+BUFFER_SIZE,0);
	}
	catch(std::bad_alloc& exception){
	  std::cerr<<"bad_alloc exception"<<std::endl;
	}
      }
    }
    //check header information
    if(request.method=="GET"||request.method=="CONNECT"){
      request.total_length=request.header_length;
      request.content_length=0;
      request.request.resize(request.total_length);
      return;
    }
    else if(request.method=="POST"){
      request.getContentlength();
      request.total_length=request.header_length+request.content_length;
      //continure receive the body
      if(request.total_length>receive_length){
	size_t remain_length=request.total_length-receive_length;
	while(remain_length!=0){
	  try{
	    request.request.resize(receive_length+BUFFER_SIZE,0);
	  }
	  catch(std::bad_alloc& exception){
	    std::cerr<<"bad_alloc exception"<<std::endl;
	  }
	  size_t recvbyte=recv(client_fd, &request.request.data()[receive_length],BUFFER_SIZE,0);
	  receive_length=receive_length+recvbyte;
	  remain_length=request.total_length-receive_length;
	}
      }
      request.request.resize(request.total_length);
      return;
    }
  }
  
  //connect to web server
  struct addrinfo * connectWebServer(Request &request){
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;
    std::string web_port="80";
    if(request.method=="CONNECT"){
      web_port="443";
    }
    //BEGIN_REF - TCP Example by by Brian Rogers, updated by Rabih Younes in ECE650                  
    status = getaddrinfo(request.hostname.c_str(), web_port.c_str(), &host_info, &host_info_list);
    if (status != 0) {
      std::cerr << "Error: cannot get address info for host" << std::endl;
      std::cerr << "  (" << request.hostname.c_str() << "," <<  web_port.c_str() << ")" << std::endl;
      throw std::exception();
      exit(EXIT_FAILURE);
    }
    webserver_fd = socket(host_info_list->ai_family,
			  host_info_list->ai_socktype,
			  host_info_list->ai_protocol);
    if (webserver_fd == -1) {
      std::cerr << "Error: cannot create socket" << std::endl;
      std::cerr << "  (" << request.hostname.c_str() << "," <<  web_port.c_str() << ")" << std::endl;
      throw std::exception();
      exit(EXIT_FAILURE);
    }
    status = connect(webserver_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      std::cerr << "Error: cannot connect to socket" << std::endl;
      std::cerr << "  (" << request.hostname.c_str() << "," <<  web_port.c_str() << ")" << std::endl;
      throw std::exception();
      exit(EXIT_FAILURE);
    }
    //END_REF                                                                                        
    return host_info_list;
  }

  //receive response helper function
  void receiveResponse(Request &request,Response &response){
    //send request to web server
    send(webserver_fd,request.request.data(),request.total_length, 0);
    //recv response from web server
    size_t receive_length=0;
    while(1){
      size_t recvbyte=recv(webserver_fd, &response.response.data()[receive_length],BUFFER_SIZE,0);
      receive_length=receive_length+recvbyte;
      if(recvbyte==0){
	break;
      }
      //receive the header
      if(response.headerEnd()==true){
	try{
	  response.parseResponse();
	  response.getContentlength();
	}
	catch(std::exception &e){
	  //502
          std::string badgate=giveResponse(request,"502","Bad Gateway");
          //send badgate to browser                                                                                                 
          send(client_fd,badgate.c_str(),badgate.length(),0);
          throw std::exception();
        }
	break;
      }
      else{
	//resize and continue receive
	try{
	  response.response.resize(receive_length+BUFFER_SIZE,0);
	}
	catch(std::bad_alloc& exception){
          std::cerr<<"bad_alloc exception"<<std::endl;
	}
      }
    }
    //check header information
    if(response.ischunk==false){
      response.total_length=response.header_length+response.content_length;
      //continue receive the body
      if(response.total_length>receive_length){
	size_t remain_length=response.total_length-receive_length;
	while(remain_length!=0){
	  try{
	    response.response.resize(receive_length+BUFFER_SIZE,0);
	  }
	  catch(std::bad_alloc& exception){
	    std::cerr<<"bad_alloc exception"<<std::endl;
	  }
	  size_t recvbyte=recv(webserver_fd, &response.response.data()[receive_length],BUFFER_SIZE,0);
	  receive_length=receive_length+recvbyte;
	  remain_length=response.total_length-receive_length;
	}
      }
      response.response.resize(response.total_length);
    }
    else{
      //handle chunk
      std::string res(response.response.begin(),response.response.end());
      std::string body=res.substr(response.header_length);
      if(body.find("\r\n\r\n")==std::string::npos){
	while(1){
	  try{
	    response.response.resize(receive_length+BUFFER_SIZE,0);
	  }
	  catch(std::bad_alloc& exception){
	    std::cerr<<"bad_alloc exception"<<std::endl;
	  }
	  size_t recvbyte=recv(webserver_fd, &response.response.data()[receive_length],BUFFER_SIZE,0);
	  receive_length=receive_length+recvbyte;
      	  std::string res(response.response.begin(),response.response.end());
	  std::string body=res.substr(response.header_length);
	  if(body.find("\r\n\r\n")!=std::string::npos){
      	    break;
	  }
	  if(recvbyte==0){
	    break;
	  }
	}
      }
      response.total_length=receive_length;
      response.response.resize(response.total_length);
    }
  }
  
  //close web server
  void closeWebServer(int webserver_fd,struct addrinfo*host_info_list){
    freeaddrinfo(host_info_list);
    close(webserver_fd);
  }
								       
  //transfer information after connect
  void transferInformation(){
    //selecct fd                                                                                     
    fd_set readfds;
    while(1){
      FD_ZERO(&readfds);
      FD_SET(client_fd,&readfds);
      FD_SET(webserver_fd,&readfds);
      int max_fd=client_fd;
      if(webserver_fd>client_fd){
	max_fd=webserver_fd;
      }
      if(select(max_fd+1,&readfds,NULL,NULL,NULL)==-1){
	std::cerr<<"select fails"<<std::endl;
	break;
      }
      if(FD_ISSET(webserver_fd,&readfds)){
	std::vector<char> transfer;
	transfer.resize(BUFFER_SIZE,0);
	size_t recvbyte_num = recv(webserver_fd, &transfer.data()[0], BUFFER_SIZE, 0);
	if(recvbyte_num==0){
	  break;
	}
	send(client_fd,transfer.data(),recvbyte_num,0);
      }
      if(FD_ISSET(client_fd,&readfds)){
	std::vector<char> transfer;
	transfer.resize(BUFFER_SIZE,0);
	size_t recvbyte_num = recv(client_fd, &transfer.data()[0], BUFFER_SIZE, 0);
	if(recvbyte_num==0){
	  break;
	}
	send(webserver_fd,transfer.data(),recvbyte_num,0);
      }
    }//while(1)                                                                                      
  }
   
  //parse the time string in response to time_t
  time_t parseToTimeT(std::string time){
    struct tm tm;
    const char * timechar=time.c_str();
    memset(&tm, 0, sizeof(struct tm));
    strptime(timechar, "%a, %d %b %Y %H:%M:%S %z", &tm);
    time_t result=mktime(&tm);
    return result;
  }
  
  //when revalidate, parse the request and add etag and last-modified time
  void parseRequest(Request& request, Response& response){
    std::string req(request.request.begin(), request.request.end());
    int pos=req.find("\r\n");
    std::string request_new;
    //check if it has etag
    if(response.etag.empty()==false){
      request_new=req.substr(0, pos)+"\r\n"+"If-None-Match: "+response.etag+"\r\n"+"If-Modified-Since: "+response.last_modified+"\r\n"+req.substr(pos+2);
    }
    else{
      request_new=req.substr(0, pos)+"\r\n"+"If-Modified-Since: "+response.last_modified+"\r\n"+req.substr(pos+2);
    }
    request.request.clear();
    request.header_length=request_new.length();
    request.total_length=request.header_length+request.content_length;
    request.request.resize(request.total_length,0);
    std::vector<char> new_request(request_new.begin(), request_new.end());
    request.request= new_request;
  }
  
  //revalidate the response, need request, old response as inputs
  void revalidate(Request request, Response response_cache,int ID,LRUCache& cache,std::ofstream& myfile){
    parseRequest(request,response_cache);
    //connet web server
    struct addrinfo * server_host_info_list;
    try{
      server_host_info_list=connectWebServer(request);
    }
    catch(std::exception &e){
      //502
      std::string badgate=giveResponse(request,"502","Bad Gateway");
      //send badgate to browser
      send(client_fd,badgate.c_str(),badgate.length(),0);
      throw std::exception();
    }
    //log
    myfile.open("/var/log/erss/proxy.log",std::fstream::app);
    myfile << ID <<": Requesting \""<< request.request_line <<"\" from "<<request.hostname<<std::endl;
    myfile.close();
    //get response from web server
    Response revalidResult;
    receiveResponse(request,revalidResult);
    //log
    myfile.open("/var/log/erss/proxy.log",std::fstream::app);
    myfile << ID <<": Received \""<< revalidResult.status <<"\" from "<<request.hostname<<std::endl;
    myfile.close();
    if(revalidResult.status_code=="200"){
      //check whether can store into cache
      if(revalidResult.cache_control.find("private")==std::string::npos&&revalidResult.cache_control.find("no-store")==std::string::npos){
	//we can store it to cache
	time_t current_time = time(nullptr);
        //set the new response to cache
        cache.set(request.request_line, revalidResult, current_time);
        //log
        if(revalidResult.cache_control.find("no-cache")!=std::string::npos||revalidResult.cache_control.find("max-age=0")!=std::string::npos){
          myfile.open("/var/log/erss/proxy.log",std::fstream::app);
          myfile << ID <<": cached, but requires re-validation"<<std::endl;
          myfile.close();
        }
	else if(revalidResult.cache_control.find("max-age")!=std::string::npos){
	  size_t pos=revalidResult.cache_control.find("=");
	  std::string max_age_str;
          size_t comma=revalidResult.cache_control.find(",",pos);
          size_t next=revalidResult.cache_control.find(",",comma+1);//next cache control
          if(next!=std::string::npos){
            max_age_str=revalidResult.cache_control.substr(pos+1,next-pos-1);
          }
          else{
            max_age_str=revalidResult.cache_control.substr(pos+1);
          }
          int max_age=stoi(max_age_str);
          time_t result=current_time+(time_t)max_age;
	  std::string expire=asctime(gmtime(&result));
          myfile.open("/var/log/erss/proxy.log",std::fstream::app);
          myfile << ID <<": cached, expires at "<<expire;
          myfile.close();
	}
	else if(revalidResult.expires.empty()==false){
	  time_t result=parseToTimeT(revalidResult.expires);
	  std::string expire=std::asctime(std::gmtime(&result));
     	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
          myfile << ID <<": cached, expires at "<<expire;
          myfile.close();
        }
        else{
          //no expire tag
          time_t date=parseToTimeT(response_cache.date);
          time_t last_modify=parseToTimeT(response_cache.last_modified);
          time_t timediff=(time_t)(date-last_modify)*0.1;
          time_t expireTime=current_time+timediff;
	  std::string expire=std::asctime(std::gmtime(&expireTime));
          myfile.open("/var/log/erss/proxy.log",std::fstream::app);
          myfile << ID <<": cached, expires at "<<expire;
          myfile.close();
	}
        //send response to web browser
	send(client_fd,revalidResult.response.data(),revalidResult.total_length,0);
	//log
        myfile.open("/var/log/erss/proxy.log",std::fstream::app);
        myfile << ID <<": Responding \""<< revalidResult.status <<"\""<<std::endl;
        myfile.close();
        //close web server
        closeWebServer(webserver_fd,server_host_info_list);
        return;
      }
      else{
        //log
        myfile.open("/var/log/erss/proxy.log",std::fstream::app);
        myfile << ID <<": not cacheable because "<<revalidResult.cache_control<<std::endl;
        myfile.close();
        //send response to web browser
	send(client_fd,revalidResult.response.data(),revalidResult.total_length,0);
        //log
        myfile.open("/var/log/erss/proxy.log",std::fstream::app);
        myfile << ID <<": Responding \""<< revalidResult.status <<"\""<<std::endl;
        myfile.close();
        //close web server
        closeWebServer(webserver_fd,server_host_info_list);
        return;
      }
    }
    if(revalidResult.status_code=="304"){
      //no content modified
      time_t current_time = time(nullptr);
      //update the time to cache
      cache.set(request.request_line, response_cache, current_time);
      //send the previous response back to browser
      send(client_fd,response_cache.response.data(),response_cache.total_length,0);
      //log
      myfile.open("/var/log/erss/proxy.log",std::fstream::app);
      myfile << ID <<": Responding \""<< response_cache.status <<"\""<<std::endl;
      myfile.close();
      //close web server
      closeWebServer(webserver_fd,server_host_info_list);
      return;
    }
  }

  //cache helper
  void cacheHelper(Request request,int ID,LRUCache& cache,std::ofstream& myfile){
    //check cache
    Response response_cache=cache.getByKey(request.request_line);
    if(response_cache.status.empty()==true){
      //not in cache
      //log
      myfile.open("/var/log/erss/proxy.log",std::fstream::app);
      myfile << ID <<": not in cache"<<std::endl;
      myfile.close();
      //connet web server
      struct addrinfo * server_host_info_list;
      try{
	server_host_info_list=connectWebServer(request);
      }
      catch(std::exception &e){
	//502
	std::string badgate=giveResponse(request,"502","Bad Gateway");
	//send badgate to browser
	send(client_fd,badgate.c_str(),badgate.length(),0);
	throw std::exception();
      }
      //log
      myfile.open("/var/log/erss/proxy.log",std::fstream::app);
      myfile << ID <<": Requesting \""<< request.request_line <<"\" from "<<request.hostname<<std::endl;
      myfile.close();
      //get resposne from web server
      Response response;
      receiveResponse(request,response);
      //log
      myfile.open("/var/log/erss/proxy.log",std::fstream::app);
      myfile << ID <<": Received \""<< response.status <<"\" from "<<request.hostname<<std::endl;
      myfile.close();
      //check whether can store response to cache
      if(response.status_code=="200" && response.cache_control.find("private")==std::string::npos && response.cache_control.find("no-store")==std::string::npos){
	//we can store it to cache
        //get the current start store time, convert it into string and store in cache
	time_t current_time = time(nullptr);
        //set the response to cache
        cache.set(request.request_line, response,current_time);
	//log
	if(response.cache_control.find("no-cache")!=std::string::npos||response.cache_control.find("max-age=0")!=std::string::npos){
	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	  myfile << ID <<": cached, but requires re-validation"<<std::endl;
          myfile.close();
	}
	else if(response.cache_control.find("max-age")!=std::string::npos){
	  size_t pos=response.cache_control.find("=");
	  std::string max_age_str;
	  size_t comma=response.cache_control.find(",",pos);
	  size_t next=response.cache_control.find(",",comma+1);//next cache control
	  if(next!=std::string::npos){
	    max_age_str=response.cache_control.substr(pos+1,next-pos-1);
	  }
	  else{
	    max_age_str=response.cache_control.substr(pos+1);
	  }
	  int max_age=stoi(max_age_str);
	  time_t result=current_time+(time_t)max_age;
	  std::string expire=asctime(gmtime(&result));
	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
          myfile << ID <<": cached, expires at "<<expire;
          myfile.close();
	}
	else if(response.expires.empty()==false){
	  time_t result=parseToTimeT(response.expires);
	  std::string expire=std::asctime(std::gmtime(&result));
      	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
          myfile << ID <<": cached, expires at "<<expire;
          myfile.close();
	}
	else{
	  //no expire tag
	  time_t date=parseToTimeT(response_cache.date);
	  time_t last_modify=parseToTimeT(response_cache.last_modified);
	  time_t timediff=(time_t)(date-last_modify)*0.1;
	  time_t currentTime = time(nullptr);
	  time_t expireTime=currentTime+timediff;
	  std::string expire=std::asctime(std::gmtime(&expireTime));
	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
          myfile << ID <<": cached, expires at "<<expire;
          myfile.close();
	}
     	//send response to web browser
	send(client_fd,response.response.data(),response.total_length,0);
	//log
	myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	myfile << ID <<": Responding \""<< response.status <<"\""<<std::endl;
	myfile.close();
	//close web server
	closeWebServer(webserver_fd,server_host_info_list);
	return;
      }
      else{
	//log
	myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	myfile << ID <<": not cacheable because "<<response.cache_control<<std::endl;
       	myfile.close();
	//send response to web browser
	send(client_fd,response.response.data(),response.total_length,0);
        //log
        myfile.open("/var/log/erss/proxy.log",std::fstream::app);
        myfile << ID <<": Responding \""<< response.status <<"\""<<std::endl;
        myfile.close();
	//close web server
	closeWebServer(webserver_fd,server_host_info_list);
	return;
      }
    }
    //in cache
    std::string control=response_cache.cache_control;
    if(control.find("no-cache")!=std::string::npos||control.find("max-age=0")!=std::string::npos){
      //revalidate every time
      //log
      myfile.open("/var/log/erss/proxy.log",std::fstream::app);
      myfile << ID <<": in cache, requires validation"<<std::endl;
      myfile.close();
      revalidate(request,response_cache,ID,cache,myfile);
      return;
    }
    else if(control.find("max-age")!=std::string::npos){
      //find max-age
      size_t pos=control.find("=");
      std::string max_age_str;
      size_t comma=control.find(",",pos);
      size_t next=control.find(",",comma+1);//next cache control
      if(next!=std::string::npos){
	max_age_str=control.substr(pos+1,next-pos-1);
      }
      else{
	max_age_str=control.substr(pos+1);
      }
      int max_age=stoi(max_age_str);
      //then check expire or not
      time_t currentTime = time(nullptr);
      time_t startTime = response_cache.start_store_time;
      time_t expireTime=startTime+(time_t)max_age;
      if(expireTime>currentTime){
	//not expire
	//log
	myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	myfile << ID <<": in cache, valid"<<std::endl;
	myfile.close();
	//send the response back to web browser
	send(client_fd,response_cache.response.data(),response_cache.total_length,0);
      	//log
        myfile.open("/var/log/erss/proxy.log",std::fstream::app);
        myfile << ID <<": Responding \""<< response_cache.status <<"\""<<std::endl;
        myfile.close();
	return;
      }
      else{
	//expires, need revalidation
        //log
	std::string expire=std::asctime(std::gmtime(&expireTime));
        myfile.open("/var/log/erss/proxy.log",std::fstream::app);
        myfile << ID <<": in cache, but expired at"<<expire;
        myfile.close();
	revalidate(request,response_cache,ID,cache,myfile);
	return;
      }
    }
    else{
      //check the expires tag
      if(response_cache.expires.empty()==false){
	time_t currentTime=time(nullptr);
	time_t expireTime=parseToTimeT(response_cache.expires);
	if(expireTime>currentTime){
	  //not expire, send the response to web browser
	  //log
	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	  myfile << ID <<": in cache, valid"<<std::endl;
	  myfile.close();
	  send(client_fd,response_cache.response.data(),response_cache.total_length,0);
	  //log
	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	  myfile << ID <<": Responding \""<< response_cache.status <<"\""<<std::endl;
	  myfile.close();
	  return;
	}
	else{
	  //expires
	  //log
	  time_t expireTime=parseToTimeT(response_cache.expires);
	  std::string expire=std::asctime(std::gmtime(&expireTime));
	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	  myfile << ID <<": in cache, but expired at"<<expire;
	  myfile.close();
	  revalidate(request,response_cache,ID,cache,myfile);
	  return;
	}
      }
      else{
	//no max-age and expires tags, calculate 10%diff(date, last-modified) as the max stroe time
	time_t date=parseToTimeT(response_cache.date);
	time_t last_modify=parseToTimeT(response_cache.last_modified);
	time_t timediff=(time_t)(date-last_modify)*0.1;
	//then check expire or not
	time_t currentTime = time(nullptr);
	time_t startTime = response_cache.start_store_time;
	time_t expireTime=startTime+timediff;
	if(expireTime>currentTime){
	  //not expire
	  //log
	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	  myfile << ID <<": in cache, valid"<<std::endl;
	  myfile.close();
	  //send the response back to web browser
	  send(client_fd,response_cache.response.data(),response_cache.total_length,0);
	  //log
	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	  myfile << ID <<": Responding \""<< response_cache.status <<"\""<<std::endl;
	  myfile.close();
	  return;
	}
	else{
	  //expires, need revalidation
	  //log
	  std::string expire=std::asctime(std::gmtime(&expireTime));
	  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
	  myfile << ID <<": in cache, but expired at"<<expire;
	  myfile.close();
	  revalidate(request,response_cache,ID,cache,myfile);
	  return;
	}
      }	
    }
  }
      

};
#endif
