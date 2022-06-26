#include <iostream>
#include <cmath>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>

#define TEN 10
#define EIGHT 8

typedef struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
}MetaData;

struct myList{
    MetaData* head;
    MetaData* last;
    size_t size;
};

myList list={nullptr, nullptr,0};

/*************************************************************************/

/***********************************************************
 ************ B Aux functions
 ***********************************************************/
//return the node of the metaData which has a free space.
MetaData* check_free_block(size_t requested_size) {
//    if(!list.head) return NULL;
//    MetaData* currPtr = list.head;
//    while (currPtr &&
//           (!(currPtr->is_free) || currPtr->size < requested_size)) {
//        currPtr = currPtr->next;
//    }
//    return currPtr;
    if(!list.head) return NULL;
    MetaData* currPtr = list.head;
    while (currPtr){
        if (currPtr->is_free && currPtr->size >= requested_size) {
            return currPtr;
        }
        currPtr = currPtr->next;
    }
    return nullptr;
}
//return the last MetaData in the list.
MetaData* find_last_block(){
//    if(!list.head) return NULL;
    MetaData* ptr = list.last;
    return ptr;

}
MetaData* my_allocate(size_t size){
    MetaData* newBlock=(MetaData*)sbrk(0);
    if ((void *) sbrk(size + sizeof(MetaData)) == (void *)-1) {
        return NULL; // sbrk failed.
    }
    newBlock->next = NULL;
    newBlock->prev = NULL;
    newBlock->size = size;
    newBlock->is_free = false;
    MetaData* last_block = find_last_block();
    if (last_block) {
        last_block->next = newBlock;
        newBlock->prev = last_block;
//        list.size++;
    }
    list.last=newBlock;//added
    return newBlock;
}
/*********************************************************/

/*1*/
void* smalloc(size_t size){
    if(size ==0 || size > pow(TEN,EIGHT)){
        return NULL;
    }
    //check if there exists a free block with the requested size
    MetaData* result = check_free_block(size);
    if(result){
        result->is_free= false;
        return result+1;
    }
    result = my_allocate(size);
    if (!result) {
        return NULL;  // sbrk failed
    }
    if(!list.head){  //if list is NULL then the list is empty
        //and we should allocate one at the start of the list
        list.head = result;
    }
    list.size++; // added
    return result + 1;

}

/*2*/
void* scalloc(size_t num, size_t size){
    size_t to_alloc=num*size;
    void* allocated= smalloc(to_alloc);
    if(!allocated){
        return nullptr;
    }
    return memset(allocated,0,to_alloc);
}

/*3*/
void sfree(void* p){
    if(!p ||((MetaData*)p -1)->is_free) return;
    ((MetaData*)p -1)->is_free= true;

}

/*4*/
void* srealloc(void* oldp, size_t size){
    if(size==0 || size > pow(10,8) ){
        return nullptr;
    }
    if(!oldp){
        return smalloc(size);
    }
    MallocMetadata* old_block=(MallocMetadata*)oldp - 1;//???
    if(size <= old_block->size ){
        return oldp;
    }
    void* new_block=smalloc(size);
    if(!new_block){
        return nullptr;
    }
    //free old if success
    sfree(oldp);
    memmove(new_block,oldp,size);
    return new_block;
}

/*5*/
//return the num of the nodes that have been released.
size_t _num_free_blocks(){
    MetaData* ptr=list.head;
    size_t counter=0;
    while (ptr)
    {
        if(ptr->is_free)
            counter++;
        ptr=ptr->next;
    }
    return counter;
}

/*6*/
size_t _num_free_bytes(){
    size_t to_return=0;
    MetaData* curr_ptr=list.head;
    while(curr_ptr){
        if(curr_ptr->is_free){
            to_return+=curr_ptr->size;
        }
        curr_ptr=curr_ptr->next;
    }
    return to_return;
}

/*7*/
size_t _num_allocated_blocks(){
//    size_t cnt=0;
//    MetaData* curr_ptr=list.head;
//    while(curr_ptr){
//        cnt++;
//        curr_ptr=curr_ptr->next;
//    }
//    return cnt;
    return list.size;
}

/*8*/
size_t _num_allocated_bytes(){
    size_t to_return=0;
    MetaData* curr_ptr=list.head;
    while(curr_ptr){
        to_return+=curr_ptr->size;
        curr_ptr=curr_ptr->next;
    }
    return to_return;
}

/*9*/
size_t _num_meta_data_bytes(){
    size_t counterBytes=((_num_allocated_blocks())*(sizeof(MetaData)));
    return counterBytes;
}

/*10*/
size_t _size_meta_data(){
    size_t meta_size=sizeof(MetaData);
    return meta_size;
}