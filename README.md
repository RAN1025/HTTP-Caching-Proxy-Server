# HTTP Caching Proxy
This is an HTTP proxy which functions with GET, POST and CONNECT and caches responses to GET requests. Our cache has size of 1024, follows the rules of expiration time and revalidation and uses LRU replacement policy. The proxy can handle multiple concurrent requests with multiple threads. Our proxy would produce a proxy.log which contains the information about each requst.

## Usage
git clone `https://gitlab.oit.duke.edu/rg241/erss-hwk2-yy226-rg241.git`       
sudo `docker-compose up`        
           
Then, this HTTP caching proxy would listen at 12345.

