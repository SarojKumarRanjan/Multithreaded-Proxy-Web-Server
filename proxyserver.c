#include "proxy_parse.h"
#include <stdio.h>

#include <string.h>
#include<time.h>





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


struct cache_element {
    char* data;
    int len;
    char* url;
    time_t = last_used_time;
    //here we used the short nomenclature as defined above
    cache_element next*;
};

/*
defination of the functions which will be called to do seperate tasks
*/

/*
this function will return the linked list node of the cache elements if exists in the cache
and return that node 
this function will take url as parameter and return the cache reltated to that url if not then it will 
return nullptr

*/

cache_element* find(char* url){

}






