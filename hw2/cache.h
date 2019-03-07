#ifndef __CACHE_H__
#define __CACHE_H__

#include <list>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <ctime>
#include <time.h>
#include "request.h"
#include "response.h"
#include "proxyserver.h"

class LRUCache {
  class CacheNode {
  public:
    std::string key;//key:request line
    Response value;//value:response
  CacheNode(const std::string& k, const Response& v): key(k), value(v){}
    CacheNode(const CacheNode &rhs){
      *this = rhs;
    }
    CacheNode & operator=(const CacheNode &rhs){
      if(this!=&rhs){
	key=rhs.key;
	value=rhs.value;
      }
      return *this;
    }
    ~CacheNode(){}
    Response getValue(){
      return value;
    }
  };
  
  std::mutex cache_mtx;//std::mutex to protect cache
  std::list<CacheNode> CacheList;
  std::unordered_map<std::string, std::list<CacheNode>::iterator> CacheHashMap;
  int capacity;

 public:
 LRUCache(const int& c): capacity(c) {
    CacheList.clear();
    CacheHashMap.clear();
  }
 
  //use request line to search the response
  //if not contain, response.status.empty() is true
  Response getByKey(const std::string& key) {
    std::lock_guard<std::mutex> guard(cache_mtx);
    Response response;
    if (CacheHashMap.count(key)) {
      response = CacheHashMap[key]->value;
      //put to the beginning
      CacheList.splice(CacheList.begin(), CacheList, CacheHashMap[key]);
      CacheHashMap[key] = CacheList.begin();
    }
    //return response;
    return response;
  }

  //BEGIN_REF - LeetCode Problem: LRU cache 
  //update the LRU cache
  void set(const std::string& key,const Response& value,const time_t time) {
    std::lock_guard<std::mutex> guard(cache_mtx);
    if (CacheHashMap.count(key)) {
      //if the map contains this key, then update the value
      CacheHashMap[key]->value = value;
      CacheHashMap[key]->value.start_store_time=time;
      //put to the beginning
      CacheList.splice(CacheList.begin(), CacheList, CacheHashMap[key]);
      CacheHashMap[key] = CacheList.begin();
    }
    else {
      if ((int)CacheList.size() >= capacity) {
	//if exceed LRU cache capacity, then erase the least used one
	CacheHashMap.erase(CacheList.back().key);
	CacheList.pop_back();
      }
      CacheList.push_front(CacheNode(key, value));
      CacheHashMap[key] = CacheList.begin();
      CacheHashMap[key]->value.start_store_time=time;
    }
  }
  //END_REF

};

#endif
