#include <iostream>
#include <unistd.h>
#include <cmath>

#define TEN 10
#define EIGHT 8

void* smalloc(size_t size)
{

    if(size ==0 || size > pow(TEN,EIGHT))
        return NULL;
    void * myMalloc=sbrk(size);
    if(myMalloc==(void*)-1)
        return NULL;
    return myMalloc;
}