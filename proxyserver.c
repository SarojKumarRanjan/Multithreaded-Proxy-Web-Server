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


#define MAX_BYTES 4096 //this will be the maximum size of the data that can be stored in the cache



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
    time_t  last_used_time;
    cache_element* next;
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


 //this function will check the http version of the request
int checkHTTPversion(char* version){
  if(strcmp(version,"HTTP/1.0")==0 || strcmp(version,"HTTP/1.1")==0){
    return 1;
  }
  return 0;
}

/*
function for the errormessage
->this function will send the error message to the client
->it will take the socketId of the client
->it will take the error code

*/

int sendErrorMessage(int socketId,int error_code){
  char str[1024];
  char current_time[50];
  time_t now = time(0);

  struct tm tm = *gmtime(&now);
  strftime(current_time,sizeof(current_time),"%a, %d %b %Y %H:%M:%S %Z",&tm);

  switch (error_code)
  {
  case 400:
    sprintf(str,"HTTP/1.1 400 Bad Request\r\nDate: %s\r\nServer: Proxy-Server\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",current_time);
    send(socketId,str,strlen(str),0);
    break;
  
  case 500:
    sprintf(str,"HTTP/1.1 500 Internal Server Error\r\nDate: %s\r\nServer: Proxy-Server\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",current_time);
    send(socketId,str,strlen(str),0);
    break;

  case 501:
    sprintf(str,"HTTP/1.1 501 Not Implemented\r\nDate: %s\r\nServer: Proxy-Server\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",current_time);
    send(socketId,str,strlen(str),0);
    break;
  
  case 403:
     sprintf(str,"HTTP/1.1 403 Forbidden\r\nDate: %s\r\nServer: Proxy-Server\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",current_time);
    send(socketId,str,strlen(str),0);
    break;

  case 404:
    sprintf(str,"HTTP/1.1 404 Not Found\r\nDate: %s\r\nServer: Proxy-Server\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",current_time);
    send(socketId,str,strlen(str),0);
    break;
  
  default:
    break;
  
  
}
}


/*
this function connects to the remote server
-> remote server is the server to which we have to make the request
-> it will take the host address and the port number as the parameter
-> it will return the socketId of the remote server

*/
int connect_to_remote_server( char* host_address, int port_number){

  int remote_socketId = socket(AF_INET,SOCK_STREAM,0);

  if(remote_socketId<0){
    perror("Failed to create the socket\n");
    return -1;
  }

  struct sockaddr_in remote_add;

  bzero((char*)&remote_add,sizeof(remote_add));

  remote_add.sin_family = AF_INET;
  remote_add.sin_port = htons(port_number);

  struct hostent *server = gethostbyname(host_address);

  if(server==NULL){
    perror("Failed to get the host\n");
    return -1;
  }

  bcopy((char*)&server->h_addr_list[0],(char*)&remote_add.sin_addr.s_addr,server->h_length);

  if(connect(remote_socketId,(struct sockaddr*)&remote_add,sizeof(remote_add))<0){
    perror("Failed to connect to the remote server\n");
    return -1;
  }

  return remote_socketId;

}


/*
this function handle the request of the client
-> it will take the socketId of the client
-> it will take the request of the client
-> it will take the tempRequest of the client
-> it will return the bytes send to the client
-> it will return -1 if the request is not handled properly


*/

int handle_request(int socketId,ParsedRequest* request,char* tempRequest){

  char* buf = (char*)malloc(MAX_BYTES*sizeof(char));
  strcpy(buf,"GET");
  strcat(buf,request->path);
  strcat(buf," ");
  strcat(buf,request->version);
  strcat(buf,"\r\n");

  size_t len = strlen(buf);

  if(ParsedHeader_set(request,"Host",request->host)<0){
    printf("Failed to set the header\n");
    return -1;
  }

  if(ParsedHeader_set(request,"Connection","close")<0){
    printf("Failed to set the header\n");
    return -1;
  }

  if(ParsedRequest_unparse_headers(request,buf+len,MAX_BYTES-len)<0){
    printf("Failed to unparse the headers\n");
    return -1;
  }

  int server_port = 80;
  if(request->host!=NULL){
    server_port = atoi(request->port);

    
  }

int remote_socketId = connect_to_remote_server(request->host,server_port);

if(remote_socketId<0){
  printf("Failed to connect to the remote server\n");
  return -1;

}

int bytes_send_client = send(remote_socketId,buf,strlen(buf),0);
bzero(buf,MAX_BYTES);



 bytes_send_client = recv(remote_socketId,buf,MAX_BYTES-1,0);



char* tempbuffer = (char*)malloc(MAX_BYTES*sizeof(char));


int tempbuffersize = MAX_BYTES;
int temp_buffer_index = 0;

while(bytes_send_client>0){
  
  bytes_send_client = send(socketId,buf,strlen(buf),0);

  for(int i=0;i<bytes_send_client/sizeof(char);i++){
    tempbuffer[temp_buffer_index] = buf[i];
    temp_buffer_index++;
  }
  tempbuffersize += MAX_BYTES;
  tempbuffer = (char*)realloc(tempbuffer,tempbuffersize*sizeof(char));

if(bytes_send_client<0){
  perror("Failed to send the data to the client server\n");
  break;
}

bzero(buf,MAX_BYTES);


bytes_send_client = recv(remote_socketId,buf,MAX_BYTES-1,0);


 
}

tempbuffer[temp_buffer_index] = '\0';
free(buf);
add_cache_element(tempRequest,str(tempbuffer),tempbuffer);
free(tempbuffer);
close(remote_socketId);

return 0;


}

void *thread_fn(void *socket_new){

  sem_wait(&semaphore);//this will wait until the semaphore value is greater than 0
  int p;
  sem_getvalue(&semaphore,&p);//this will get the value of the semaphore

  printf("Semaphore value is %d\n",p);
   int *t = (int*)socket_new;
   int socketId = *t;
   int bytes_send_client,length;

   char *buffer = (char*)calloc(MAX_BYTES,sizeof(char));

   bzero(buffer,MAX_BYTES);

   bytes_send_client = recv(socketId,buffer,MAX_BYTES,0);

   while(bytes_send_client>0){
    length = strlen(buffer);

    if(strstr(buffer,"\r\n\r\n")==NULL){
      bytes_send_client = recv(socketId,buffer+length,MAX_BYTES-length,0);
    }else{
      break;
    }
   }

/*
Here tempRequest will be the copy of the buffer for the purpose of the parsing the request
*/


   char *tempRequest = (char*)malloc(strlen(buffer)*sizeof(char)+1);

//copy the buffer to the tempRequest
   for(int i=0;i<strlen(buffer);i++){
    tempRequest[i] = buffer[i];
   }


//Here we are finding the cache element related to the request

struct cache_element* temp = find(tempRequest);

//if found then send the data to the client

if(temp!=NULL){
  printf("Data found in the cache\n");
  int size = temp->len/sizeof(char);

  int pos = 0;
  char response[MAX_BYTES];

  /*
  this while loop will send the data to the client in the chunks of MAX_BYTES
  -first it checks the position of the data in the cache
  -then it will increment the position of the data in the cache
  -then it will send the data to the client

  */

  while(pos<size){
    bzero(response,MAX_BYTES);
    
    for(int i=0;i<MAX_BYTES;i++){
      response[i] = temp->data[pos];
      pos++;
      
      send(socketId,response,MAX_BYTES,0);


    }

    printf("Data found in the cache\n");
    printf("%s\n\n",response);
  }



}else if(bytes_send_client > 0){
  length = strlen(buffer);
  //parse the request
  ParsedRequest* request = ParsedRequest_create();

  if(ParsedRequest_parse(request,buffer,length)<0){
    printf("Parsing failed\n");
  }else{
    bzero(buffer,MAX_BYTES);
    //get the method of the request
    //if the method is GET then we will send the request to the server
    //currently we are not handling the POST,PUT,DELETE methods
    if(!strcmp(request->method,"GET")){

        if(request->host && request->path && checkHTTPversion(request->version)==1){
          bytes_send_client = handle_request(socketId,request,tempRequest);
          if(bytes_send_client==-1){
            sendErrorMessage(socketId,500);
          }
        }else{
          sendErrorMessage(socketId,500);
        }
    }
    else{
      printf("Only GET method is allowed\n");
    }
  }

  ParsedRequest_destroy(request);


}

else if(bytes_send_client == 0){
  printf("Client disconnected\n");

}
shutdown(socketId,SHUT_RDWR);
close(socketId);
free(buffer);
sem_post(&semaphore);

sem_getvalue(&semaphore,&p);

printf("Semaphore post value is %d\n",p);

free(tempRequest);

return NULL; 
}

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
      if(setsockopt(proxy_socketId,SOL_SOCKET,SO_REUSEADDR,(const char* )&reuse,sizeof(reuse))<0){
        perror("setsocketopt (SO_REUSEADDR) failed"); 
      }

      bzero((char*)&server_add , sizeof(server_add));

      server_add.sin_family = AF_INET;
      server_add.sin_port = htons(port_number);
      server_add.sin_addr.s_addr = INADDR_ANY;

      if(bind(proxy_socketId,(struct sockaddr*)&server_add,sizeof(server_add))<0){
        perror("Port is not available for the binding\n");
        exit(1);
      }

      printf("Binding to port %d\n",port_number);


      int listen_status = listen(proxy_socketId,MAX_CLIENT);

      if(listen_status<0){
        perror("Error in listening port\n");
        exit(1);

      }

      int i=0;
      int connected_socketId[MAX_CLIENT];

      while(1){
    bzero((char* )&client_add,sizeof(client_add));
    client_length = sizeof(client_add);
    client_socketId = accept(proxy_socketId,(struct sockaddr *)&client_add,(socklen_t*)&client_length);

       if(client_socketId<0){
        printf("Not able to connect due to client socketID not initialized");
        exit(1);
       }

       else{
        connected_socketId[i] = client_socketId;

       }

       struct sockaddr_in * client_pt = (struct sockaddr_in * )&client_add;
       struct in_addr ip_addr = client_pt ->sin_addr;
       char str[INET_ADDRSTRLEN];
       inet_ntop(AF_INET,&ip_addr,str , INET_ADDRSTRLEN);

       printf("Client is connected on the port %d with ip address %s\n",ntohs(client_add.sin_port),str);


       pthread_create(&tid[i],NULL,thread_fn,(void *)&connected_socketId[i]);

       i++;

    
      }

      close(proxy_socketId);
return 0;
}



cache_element* find(char* url){

  cache_element* temp = NULL;

  int time_lock_val = pthread_mutex_lock(&lock);

  printf("Time lock value is %d\n",time_lock_val);

   temp = head;
  while(temp!=NULL){
    if(strcmp(temp->url,url)==0){
      temp->last_used_time = time(0);
      return temp;
    }
    temp = temp->next;
  }
  return NULL;
}


cache_element* create_cache_element(char* url,int size,char* data){

  int time_lock_val = pthread_mutex_lock(&lock);

  printf("Create cahe  Time lock value is %d\n",time_lock_val);

  cache_element* temp = (cache_element*)malloc(sizeof(cache_element));
  temp->url = (char*)malloc(strlen(url)*sizeof(char)+1);
  strcpy(temp->url,url);
  temp->data = (char*)malloc(size*sizeof(char)+1);
  strcpy(temp->data,data);
  temp->len = size;
  temp->last_used_time = time(0);
  temp->next = NULL;
  return temp;
}


void remove_cache_element(){

  int time_lock_val = pthread_mutex_lock(&lock);

  printf("Remove cache Time lock value is %d\n",time_lock_val);

  cache_element* temp = head;
  cache_element* prev = NULL;
  cache_element* min = NULL;
  time_t min_time = time(0);

  while(temp!=NULL){
    if(temp->last_used_time<min_time){
      min_time = temp->last_used_time;
      min = temp;
      prev = temp;
      temp = temp->next;
    }else{
      prev = temp;
      temp = temp->next;
    }
  }

  if(min!=NULL){
    if(min==head){
      head = head->next;
    }else{
      prev->next = min->next;
    }
    free(min->url);
    free(min->data);
    free(min);
  }
}











