#include "proxyserver.h"
#include "cache.h"
#include "response.h"
#include "request.h"

//handle the request
void helper (Proxy proxy,int ID,LRUCache& cache,std::ofstream& myfile){
  //receive request from web browser
  Request request;
  try{
  proxy.receiveRequest(request);
  }
  catch(std::exception &e){
    return;
  }
  //get current time
  time_t current_time = time(nullptr);
  std::string time=asctime(gmtime(&current_time));
  //log
  myfile.open("/var/log/erss/proxy.log",std::fstream::app);
  myfile << ID <<": \""<< request.request_line <<"\" from "<<request.IP<<" @ "<<time;
  myfile.close();
  if(request.method=="GET"){
    //use cache
    try{
      proxy.cacheHelper(request,ID,cache,myfile);
    }
    catch(std::exception &e){
      return;
    }
  }
  if(request.method=="POST"){
    //connet web server
    struct addrinfo * server_host_info_list=proxy.connectWebServer(request);
    //log
    myfile.open("/var/log/erss/proxy.log",std::fstream::app);
    myfile << ID <<": Requesting \""<< request.request_line <<"\" from "<<request.hostname<<std::endl;
    myfile.close();
    //get response from web server
    Response response;
    try{
      proxy.receiveResponse(request,response);
    }
    catch(std::exception &e){
      return;
    }
    //log
    myfile.open("/var/log/erss/proxy.log",std::fstream::app);
    myfile << ID <<": Received \""<< response.status <<"\" from "<<request.hostname<<std::endl;
    myfile.close();
    //send response to web browser
    send(proxy.client_fd,response.response.data(),response.total_length,0);
    //log
    myfile.open("/var/log/erss/proxy.log",std::fstream::app);
    myfile << ID <<": Responding \""<< response.status <<"\""<<std::endl;
    myfile.close();
    //close web server
    proxy.closeWebServer(proxy.webserver_fd,server_host_info_list);
  }
  if(request.method=="CONNECT"){
    //connect web server
    struct addrinfo * server_host_info_list=proxy.connectWebServer(request);
    //log      
    myfile.open("/var/log/erss/proxy.log",std::fstream::app);
    myfile << ID <<": Requesting \""<< request.request_line <<"\" from "<<request.hostname<<std::endl;
    myfile.close();
    std::string ok="HTTP/1.1 200 Ok\r\n\r\n";
    //send ok to browser
    send(proxy.client_fd,ok.c_str(),ok.length(),0);
    //log
    myfile.open("/var/log/erss/proxy.log",std::fstream::app);
    myfile << ID <<": Responding \"HTTP/1.1 200 Ok\""<<std::endl;
    myfile.close();
    //transfer information between browser and web server
    proxy.transferInformation();
    //close web server
    proxy.closeWebServer(proxy.webserver_fd,server_host_info_list);
    //log
    myfile.open("/var/log/erss/proxy.log",std::fstream::app);
    myfile << ID <<": Tunnel closed"<<std::endl;
    myfile.close();
  }
}

LRUCache cache(1024);

int main(){
  Proxy proxy("12345");
  //connect to web browser
  struct addrinfo * browser_host_info_list=proxy.connectWebBrowser();
  int ID=0;
  std::ofstream myfile;
  myfile.open("/var/log/erss/proxy.log",std::ofstream::out | std::ofstream::trunc);
  myfile.close();
  while(1){
    ID++;
    //accept connetion from browser
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    proxy.client_fd = accept(proxy.proxy_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (proxy.client_fd == -1) {
      std::cerr << "Error: cannot accept connection on socket" << std::endl;
      continue;
    }
    std::thread th(helper,proxy, ID,std::ref(cache),std::ref(myfile));
    th.detach();
  }//while(1）
  freeaddrinfo(browser_host_info_list);
  close(proxy.proxy_fd);
  return EXIT_SUCCESS;
}
