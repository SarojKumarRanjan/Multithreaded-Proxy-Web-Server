#include "proxy_parse.h"
#include <stdio.h>
#include<pthread.h>
#include <string.h>
#include<time.h>

#include<sys/socket.h>

#include<sys/types.h>

#include<stdlib.h>

#include<netdb.h>

#include<netinet/in.h>

#include<arpa/inet.h>

#include<unistd.h>

#include<fcntl.h>

#include<sys/wait.h>

#include<errno.h>

#include<semaphore.h>


/*
what is pthread?
->

standardized way to create and manage threads, which are lightweight units of execution within a single process. 
 It's crucial for writing concurrent programs that can take advantage of multi-core processors or 
 perform multiple tasks seemingly simultaneously.

 usage of pthread : 

 Parallelism: On multi-core systems, threads can run on different cores simultaneously, significantly 
              speeding up computationally intensive tasks.   
Responsiveness: In applications like graphical user interfaces (GUIs), one thread can handle user input 
                 while other threads perform background tasks,
                 keeping the application responsive.   
Concurrency: Even on single-core systems, threads can be used to manage multiple tasks concurrently.
             While they might not be truly parallel, the operating system can switch between them rapidly,
             giving the illusion of simultaneous execution and improving overall efficiency.   
Simplified Design: Some problems are naturally expressed as multiple concurrent tasks.
                    Using threads can simplify the design and implementation of such applications.

*/


//define typedef for the cache element so that we do not have to rewrite it everytime

typedef struct cache_element cache_element;

/*
number of clients that can connect to my proxy server at a time 
concurrent clients
*/

#define MAX_CLIENT 20



/*
Defining the structure of the chache element
this will be a linked list of the cache element
the lru will be defined on the basis of time
*/


 //here we used the short nomenclature as defined above
struct cache_element {
    char* data;
    int len;
    char* url;
    time_t = last_used_time;
    cache_element next*;
};

/*
defination of the functions which will be called to do seperate tasks
*/

/*
this function will return the linked list node of the cache elements if exists in the cache
and return that node 
this function will take url as parameter and return the cache reltated to that url 
if not then it will return nullptr

*/

cache_element* find(char* url);

/*
this function will add the coming requests to the cache with its relevent data 

*/

int add_cache_element(char* url, int size , char* data);
/*
this function will remove the request's data from the cache linked list 

*/

void remove_cache_element();

// define port number for the proxy server to run 

int port_number = 4000;
int proxy_socketId;

/*
pthread_t is a data type used to represent a thread. It's essentially an identifier for a thread within a process.
*/

pthread_t tid[MAX_CLIENT];


/*
Purpose: To control access to a limited number of resources.
          It acts as a counter that tracks the availability of resources.  

Mechanism: Uses a signaling mechanism with two atomic operations:

sem_wait() (or P): Decrements the semaphore value. If the value becomes negative,
                   the thread blocks until it becomes non-negative.
sem_post() (or V): Increments the semaphore value, potentially unblocking a waiting thread.


it will descrese the value when a thread acquire the process .
 lets suppose a all thread bloked then its value will be negative 
then it will not allow the next thred to do their work

*/

sem_t semaphore;

/*
 To protect a critical section of code, ensuring that only one thread can access it at a time.

 Think of it as a lock that only one thread can hold at any given moment.   

*/

pthread_mutex_t lock;



/*
define the head of the cache_element
*/

cache_element* head;

int cache_size; // this will denote the current size of the cache


int main(int argc , char* argv[]){

    //define the client socketID and the client_length

    int client_socketId,client_length;

    struct sockaddr_in client_add,server_add; //this represent the the address of the client and the server 
                                          //to which i have to make request;

    sem_init(&semaphore , 0 , MAX_CLIENT);
    pthread_mutex_init(&lock,NULL);
    if(argc == 2){
        //./proxy-server 9090 it will take two argument file and the port number to run 

        port_number = atoi(argv[1]);
    }else{
        printf("Give proper arguments\n");
        exit(1);
    }

    printf("starting server on the port: %d\n",port_number);

    /*

   AF_INET :  IPv4 Internet protocols
   SOCK_STREAM : Provides sequenced, reliable, two-way, connection-based byte streams.
                 An out-of-
                 band data transmission mechanism may be supported.
    

    */

      proxy_socketId = socket(AF_INET,SOCK_STREAM,0);

      if(proxy_socketId<0){
        perror("Failed to create to socker\n");
        exit(1);
      }

      int reuse = 1;
      if(setsocketopt(proxy_socketId,SOL_SOCKET,SO_REUSEADDR,(const char* )&reuse,sizeof(reuse))<0){
        perror("setsocketopt (SO_REUSEADDR) failed"); 
      }

      bzero((char*)&server_add , sizeof(server_add));

      







return 0;
}















