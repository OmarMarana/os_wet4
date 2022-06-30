#include <sys/mman.h>
#include <iostream>
#include <cmath>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>

#define TEN 10
#define EIGHT 8
#define LONG 128*1024

typedef struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
    MallocMetadata* next_free;
    MallocMetadata* prev_free;
}MetaData;

struct myList{
    MetaData* head;
    MetaData* last;
    size_t size;
};

myList list={nullptr, nullptr,0};
myList listMax={nullptr, nullptr,0};
MetaData* bin_array[128];

/***********************************************************
 ************ Main functions - Declaration
 ***********************************************************/
void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();
/***********************************************************
 ************ B Aux functions
 ***********************************************************/
int get_bin_index(size_t size){
    if(size == 128 * 1024){
        return 127;
    }
    int bin = size/1024 ;
    return bin;
}

//return the node of the metaData which has a free space.
MetaData* check_free_block(size_t requested_size, int* index) {
    if(!list.head) return nullptr;
    int bin_index = get_bin_index(requested_size);
    for(int i=bin_index; i<128;i++){
        MetaData* head_list = bin_array[i];
        MetaData* curr = head_list;
        while(curr){
            if(curr->size >= requested_size){
                *index = i ;
                return curr;
            }
            curr=curr->next_free;
        }
    }
    return nullptr;
}
//return the last MetaData in the list.
MetaData* find_last_block(){
    MetaData* curr=list.head;
    if(!curr){
        return nullptr;
    }
    while(curr->next){
        curr=curr->next;
    }
    return curr;
//    if(!list.head) return NULL;
//    MetaData* ptr = list.last;
//    return ptr;

}
MetaData* find_last_blockMax(){
//    if(!listMax.head) return NULL;
//    MetaData* ptr = listMax.last;
//    return ptr;
    MetaData* curr=listMax.head;
    if(!curr){
        return nullptr;
    }
    while(curr->next){
        curr=curr->next;
    }
    return curr;
}
///////??????????????????????????

////////////////////////////
//allocation for Max and regular size.
MetaData* my_allocate(size_t size){
    bool flag= false;
    MetaData* newBlock;
    if(size > LONG) {
        flag = true;
    }
    if(flag==true){
        newBlock= (MetaData*) mmap(NULL, size+ sizeof(MetaData),
                                   PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
        if((void*) newBlock == (void *) -1) return (NULL);
    }
    else{
        newBlock=(MetaData*)sbrk(0);
        if ((void *) sbrk(size + sizeof(MetaData)) == (void *)-1) {
            return nullptr; // sbrk failed.
        }
    }
    newBlock->next = nullptr;
    newBlock->prev = nullptr;
    newBlock->next_free = nullptr;
    newBlock->prev_free = nullptr;
    newBlock->size = size;
    newBlock->is_free = false;
    MetaData* last_block;
    if(flag==true) {
        last_block = find_last_blockMax();
        if (last_block) {
            last_block->next = newBlock;
            newBlock->prev = last_block;
        }
        listMax.last = newBlock;//added
        return newBlock;
    }
    else {
        last_block=find_last_block();
    }
    if (last_block) {
        last_block->next = newBlock;
        newBlock->prev = last_block;
    }
    list.last=newBlock;//added
    return newBlock;
}

//split a free block into 2.
//challenge **1**
void add_to_bin_array(MetaData** to_add1){ //TODO: sort by address
    MetaData* free_block = *to_add1;
    //add the free block to the bin_array
    size_t p_size = free_block->size;
    int bin_index = get_bin_index(p_size);
    if(bin_index>=128){ //TODO: can occur?
        return;
    }
    MetaData* head = bin_array[bin_index];
    MetaData* curr = head;
    
    //check if the list is empty
    if(!head){
        bin_array[bin_index] = free_block;
        free_block->next_free = nullptr;
        free_block->prev_free = nullptr;
        return;	
    }
    MetaData* prev = curr->prev_free;
    while((curr != NULL) && ((curr->size < p_size) ||  (curr < free_block &&  curr->size == p_size )))
    {
        curr = curr->next_free;
        prev = curr->prev_free;

    }
    //if reached here then prev isnt nullptr
    if(curr==NULL)
    {

        free_block->prev_free =prev;
        free_block->next_free = prev->next;
        prev->next_free = free_block;
        return;
    }
    else
    {
        free_block->prev_free =curr->prev_free;
        free_block->next_free = curr;
        curr->prev_free = free_block;
        return;
    }


    // while(curr){      // sort by address
    //     MetaData* next = curr->next_free;
    //     if(curr->size <= p_size){
    //         if(next){
    //             if(p_size <= next->size){
    //                 free_block->next_free = next;
    //                 free_block->prev_free = curr;
    //                 next->prev_free = free_block;
    //                 curr->next_free = free_block;
    //                 return;
    //             }
    //         }else{
    //             free_block->next_free = next;
    //             free_block->prev_free = curr;
    //             curr->next_free = free_block;
    //             return;
    //         }
    //     }
    //     curr = next;
    // }
    
    // end of adding to bin_array
}
MetaData* split(MetaData* currBlock,size_t size){//?????????????
    if(currBlock->size < size + 128 + _size_meta_data()){//???
        if(currBlock->is_free){
            currBlock->is_free = false;
        }
        return currBlock+1;//????
    }
    MetaData* free_block = (MetaData*)(((char *)currBlock) + size +_size_meta_data());
    free_block->size = currBlock->size - _size_meta_data()- size;
    free_block->is_free = true;
    free_block->next = currBlock->next;
    free_block->prev=currBlock;
    if(free_block->next){ // TODO: check we added
        (free_block->next)->prev = free_block;
    }
    currBlock->next = free_block;
    currBlock->is_free = false;
    currBlock->size = size;
    //add the free block to the bin_array
    add_to_bin_array(&free_block);
    /*
    size_t p_size = free_block->size;
    int bin_index = get_bin_index(p_size);
    if(bin_index>=128){
        return currBlock+1;
    }
    MetaData* head = bin_array[bin_index];
    MetaData* curr = head;
    while(curr){
        MetaData* next = curr->next;
        if(curr->size <= p_size){
            if(next){
                if(p_size <= next->size){
                    free_block->next_free = next;
                    free_block->prev_free = curr;
                    next->prev_free = free_block;
                    curr->next_free = free_block;
                }
            }else{
                free_block->next_free = next;
                free_block->prev_free = curr;
                curr->next_free = free_block;
            }
        }
    }
    //check if the list is empty
    if(!head){
        bin_array[bin_index] = free_block;
        free_block->next_free = nullptr;
        free_block->prev_free = nullptr;
    }
    */
    // end of adding to bin_array
    //update last ??? if was currBlock=lat => now last=freeBlock
    return currBlock+1;
}
void update_free_links(MetaData** node1){
    MetaData* node = *node1;
    int bin_index = get_bin_index(node->size);
    if(bin_index > 127){ //TODO: that will not occur right?
        return;
    }
    MetaData* prev_free = node->prev_free;
    MetaData* next_free = node->next_free;
    node->next_free = nullptr;
    node->prev_free = nullptr;
    if(prev_free){
        prev_free->next_free = next_free;
    }else{ // was first element in the list bin_array[i]
        bin_array[bin_index] = next_free;
    }
    if(next_free){
        next_free->prev_free = prev_free;
    }
}
//omar: enlarge wilderness chunk
MetaData * checkChunck(size_t size){//??????
    MetaData* chunkNode=find_last_block();//list.last;
    if(chunkNode->is_free){
        chunkNode->is_free = false;
        update_free_links(&chunkNode); // TODO: test
        void* result = sbrk(size-chunkNode->size);//sbrk(size+chunkNode->size);//????check
        if(*(int*)result == -1) {
            chunkNode->is_free = true;
            add_to_bin_array(&chunkNode); // TODO: test
            return nullptr;
        }
        chunkNode->size = size;
//        chunkNode->is_free = false;
//        update_free_links(&chunkNode); // TODO: test
//        chunkNode->next_free = nullptr;
//        chunkNode->prev_free = nullptr;
        return chunkNode+1;
    }
    return nullptr; //added
}
void merge(MetaData* currBlock){
    size_t tempSize;
    MetaData* prevPtr=currBlock->prev;
    MetaData* NxtPtr=currBlock->next;
    if(currBlock->size > LONG){ //TODO: they are gonna check this case? that free block will be LONG
//        munmap(currBlock,currBlock->size + _size_meta_data());
//        std::cout<< "curr size= " << currBlock->size;
        if (NxtPtr) { /// make sure that all cases exist ???
//            std::cout<< "curr size= " << currBlock->size<<" ,next size= "<<NxtPtr->size; //delete
            NxtPtr->prev = currBlock->prev;
            if (prevPtr) {
//                std::cout<< "prev size= " << prevPtr->size;//delete
                prevPtr->next = NxtPtr;
            } else {
                listMax.head = NxtPtr;
            }
        }else {
            listMax.last = prevPtr;

            if (prevPtr) {
                prevPtr->next = NxtPtr;

            } else {
                listMax.head= NULL;
            }
        }
        munmap(currBlock,currBlock->size + _size_meta_data());
        return;
    }
    //omar:merge all three blocks into one block
    if(NxtPtr&& prevPtr &&NxtPtr->is_free && prevPtr->is_free) {
        tempSize = prevPtr->size + NxtPtr->size + currBlock->size + (2 * _size_meta_data());
        prevPtr->next = NxtPtr->next;
        if(prevPtr->next){//added
            prevPtr->next->prev=prevPtr;//added
        }else{//added
            list.last=prevPtr;//added
        }//added
        prevPtr->size=tempSize;
        // update the bin_array,
        // prev is free so he was in the bin_array
        // but now we update the free pointers
        // add him to bin_array like he was'nt there,
        // delete him and insert him again so he will find the appropriate place
        // we need to delete the next too from the bin_array
        update_free_links(&NxtPtr);
        update_free_links(&prevPtr);
        //add the prev to bin_array
        add_to_bin_array(&prevPtr);
        return;
    }
    if(NxtPtr && NxtPtr->is_free){
        tempSize=currBlock->size+NxtPtr->size+ _size_meta_data();
        currBlock->next=NxtPtr->next;
        if(NxtPtr->next){ //added
            NxtPtr->next->prev=currBlock;//added
        } else{//added
            list.last=currBlock;//added
        }//added
//        currBlock->size=tempSize; // TODO: test
        // now we should update free pointers
        // we are gonna exit next&curr from the bin_array
        update_free_links(&NxtPtr);
        update_free_links(&currBlock);
        // curr wasn't in bin_array so we are gonna insert
        currBlock->size=tempSize;
        add_to_bin_array(&currBlock);
        return;
    }
    if(prevPtr && prevPtr->is_free){
        tempSize=currBlock->size+prevPtr->size+ _size_meta_data();
        prevPtr->next=currBlock->next;
        if(currBlock->next){//added
            currBlock->next->prev=currBlock->prev;//added
        }else{//added
            list.last=currBlock->prev;//added
        }//added
        prevPtr->size=tempSize;
        // now we should update free pointers
        // we are gonna exit prev from the bin_array
        update_free_links(&prevPtr);
        //we are gonna insert
        add_to_bin_array(&prevPtr);
        return;
    }

}
/*********************************************************/


/***********************************************************
 ************ Sondos Aux functions
 ***********************************************************/
void* LongReAllocate(void* oldp,size_t size) {
    MetaData *old_block = (MetaData *) oldp - 1;
    size_t old_size_to_copy = old_block->size;
    size_t old_size=old_block->size+_size_meta_data();
    MetaData* prev_block=old_block->prev;
    MetaData* next_block=old_block->next;
    MetaData *newBlock;
    newBlock = (MetaData *) mmap(NULL, size + sizeof(MetaData),
                                 PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if ((void *) newBlock == (void *) -1) {
        return nullptr;
    }

    newBlock->next = nullptr;
    newBlock->prev = nullptr;
    newBlock->size = size;
    newBlock->is_free = false;//???

    old_block->size = size;
    if(!prev_block && !next_block){ //the newBlock is the head and last
        listMax.head=newBlock;
    }
    if(prev_block){
        prev_block->next=newBlock;
    }
    if(next_block){
        next_block->prev=newBlock;
    }
    newBlock->prev=prev_block;
    newBlock->next=next_block;
    //munmap(oldp,old_size);
    //
    void* newOldp = newBlock +1; //???
    int toCopy=(old_size_to_copy<size)? old_size_to_copy:size;
    void* aa = memmove(newOldp,oldp,toCopy);
    munmap(oldp,old_size);
    return aa;
    //
    //return memmove(res,oldp,size);
}

void* largeEnough(void* ptr,size_t size){
    MetaData* ptr_block=(MetaData*)ptr-1;
    size_t diff=ptr_block->size-size;
    if(diff <= _size_meta_data()){
        return ptr;
    }
    diff-=_size_meta_data();
    if(ptr_block->size > size && diff>=128){//???
        char* tmpBlock= ((char*)ptr)+size;
        MetaData* block= (MetaData*) tmpBlock;
        //block->is_free=true;//???????????????????? TODO: check
        block->size=diff;
        ptr_block->size=size;
        MetaData* ptr_next=ptr_block->next;
        ptr_block->next=block;
        block->next=ptr_next;
        block->prev=ptr_block;//TODO: test
        if(block->next){//TODO: test
            (block->next)->prev=block;//TODO: test
        }//TODO: test
        //sfree(block+1); // TODO: test ask ?????
        block->is_free= true; // TODO: test ask ?????added
        add_to_bin_array(&block);//TODO: test ask ?????
        return ptr;
    }
    return ptr;
}
void* reallocEdgeCases(void* oldp,MetaData* curr_block,size_t size){
    MetaData* prev_block=curr_block->prev;
    MetaData* next_block=curr_block->next;
    /// b. Try to merge with the adjacent block with the lower address.
    if(prev_block && prev_block->is_free){
        size_t requested_size=curr_block->size+prev_block->size+_size_meta_data();
        if(requested_size >= size){
            prev_block->is_free= false; //TODO: test
            // delete prev from bin array //TODO: test
            update_free_links(&prev_block); //TODO: test
            prev_block->size+=curr_block->size+_size_meta_data();
            prev_block->next=next_block;
            if(next_block){
                next_block->prev=prev_block;
            }else{
                list.last=prev_block;
            }
//            prev_block->is_free= false;
//            // delete prev from bin array
//            update_free_links(&prev_block);
            if(requested_size==size){
                ////////////////////
                memmove((MetaData*)prev_block+1,oldp,size);
                ////////////////////
                return (MetaData*)prev_block+1; //changed baraah
            }
            void* newOldp = (char*) (prev_block +1); //???
            int toCopy=(curr_block->size<size)? curr_block->size:size;
            memmove(newOldp,oldp,toCopy);
            return largeEnough(newOldp,size);
        }
    }
    /// c. Try to merge with the adjacent block with the higher address.
    if(curr_block->next && curr_block->next->is_free){
        size_t requested_size=curr_block->size+next_block->size+_size_meta_data();
        if(requested_size >= size){
            (curr_block->next)->is_free= false;
            // delete prev from bin array
            update_free_links(&(curr_block->next));

            curr_block->size += next_block->size + _size_meta_data();
            next_block=next_block->next;
            curr_block->next=next_block;
            if(next_block){ //else curr=last ??
                next_block->prev=curr_block;
            }else{
                list.last=curr_block;
            }
            if(requested_size==size){
                memmove((MetaData*)curr_block+1,oldp,size);//changed baraah
                return oldp;
            }
            return largeEnough(oldp,size);
        }
    }
    /// d. Try to merge all those three adjacent blocks together.
    if(prev_block && prev_block->is_free && next_block && next_block->is_free){
        size_t requested_size=curr_block->size+prev_block->size+next_block->size+_size_meta_data()+2*_size_meta_data();
        if(requested_size >= size){
            // delete prev&next from bin_array
            update_free_links(&prev_block);
            update_free_links(&next_block);
            //
            prev_block->size+=curr_block->size+next_block->size+2*_size_meta_data();
            next_block=next_block->next;
            prev_block->next=next_block;
            if(next_block){
                next_block->prev=prev_block;
            }else{//sure???
                list.last=prev_block;
            }
            prev_block->is_free=false;
            if(requested_size==size){
                return prev_block;
            }
            void* newOldp = (MetaData *) (prev_block + 1);
            int toCopy=(curr_block->size<size)? curr_block->size:size;
            memmove(newOldp,oldp,toCopy);//changed
            return largeEnough(newOldp,size);
        }
    }

    if(!next_block){//which case ??? piazza to enlarge if last ?????????????????????????????????? check
        size_t requested_size=size-curr_block->size;
        void* allocated=sbrk(requested_size);
        if(allocated == (void *) -1){
            return nullptr;
        }
        curr_block->size=size;
//        curr_block->is_free=false;//???
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


/*1*/
void* smalloc(size_t size){
    if(size ==0 || size > pow(TEN,EIGHT)){
        return NULL;
    }
    //needed change
    size_t requested_size8=size+_size_meta_data();
    if(requested_size8 % 8 !=0){
        size_t diff=8-(requested_size8%8);
        requested_size8+= diff;
    }
    if(size > LONG) // TODO: Make sure check >= ??
    {
        MetaData* result=my_allocate(requested_size8-_size_meta_data());//(size);
        if(!listMax.head) {
            listMax.head = result;
        }
        listMax.size++;
        return result+1;
    }
    if(!list.head){  //if listptr is NULL then the list is empty
        //and we should allocate one at the start of the list
        MetaData* result = my_allocate(requested_size8-_size_meta_data());//(size);
        list.head = result;
        return result + 1;
    }
    //check if there exists a free block with the requested size
    int bin_index;
    MetaData* result = check_free_block(requested_size8-_size_meta_data(),&bin_index);//(size, &bin_index);
    if(result){
        result->is_free= false;
        update_free_links(&result);
        return split(result, requested_size8-_size_meta_data());//size);
    }
    // check the chunk wilderness
    //********************/
    result=checkChunck(requested_size8-_size_meta_data());//(size);
    if(result!= nullptr) {
        return result;
    }

    //*********************//
    //we should allocate for the current size.
    result = my_allocate(requested_size8-_size_meta_data());//(size);
    if (!result) {
        return nullptr;  // sbrk failed
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
    MetaData* freed = (MetaData*)p -1;
    add_to_bin_array(&freed);
    merge((MetaData*)p -1);
}

/*4*/
void* srealloc(void* oldp, size_t size){
    if(size==0 || size > pow(10,8) ){
        return nullptr;
    }
    //needed change
    size_t requested_size8=size+_size_meta_data();
    if(requested_size8 % 8 !=0){
        size_t diff=8-(requested_size8%8);
        requested_size8+= diff;
    }
    size_t new_size=requested_size8-_size_meta_data();
    if(!oldp){
        return smalloc(new_size);//(size);
    }
    MallocMetadata* old_block=(MallocMetadata*)oldp - 1;//???
    /////
    //a. Try to reuse the current block without any merging
    if(/*size*/new_size == old_block->size ){ //TODO: check <= changed == , before/after next if ?
        return oldp;
    }
    /////
    //is this case possible??? yes sondos
    if(/*size*/new_size>LONG && old_block->size>LONG){ // TODO: test
        return LongReAllocate(oldp,/*size*/new_size);
    }
    //maybe we need to split
    if(/*size*/new_size<=old_block->size){
        size_t num_before_split=_num_allocated_blocks();
        split(old_block,/*size*/new_size);
        size_t num_after_split=_num_allocated_blocks();
        if(num_before_split != num_after_split){ //if true split ocurred
            if(old_block->next->next && old_block->next->next->is_free){
                //merge old->next with next
                MetaData* merge1=old_block->next;
                MetaData* merge2=merge1->next;
                merge1->size += merge2->size+_size_meta_data();
                if(merge2->next){
                    merge2->next->prev=merge1;
                }else{
                    list.last=merge1;
                }
                merge1->next=merge2->next;
                // delete merge 2 from bin array
                update_free_links(&merge2);
                // delete merge 1 from bin array and insert again
                update_free_links(&merge1);
                add_to_bin_array(&merge1);
            }
        }
        return oldp;
    }
    // try to merge with adjacents
    if(old_block->size < /*size*/new_size){
        return reallocEdgeCases(oldp,old_block,/*size*/new_size);
    }
    /////
    void* new_block=smalloc(/*size*/new_size);
    if(!new_block){
        return nullptr;
    }
    //free old if success
    sfree(oldp);
    memmove(new_block,oldp,/*size*/new_size);
    return new_block;
}

/*5*/
//return the num of the nodes that have been released.
size_t _num_free_blocks(){
    MetaData* ptr=list.head;
    // MetaData*ptrMax=listMax.head;
    size_t counter=0;
    while (ptr)
    {
        if(ptr->is_free) {
            counter++;
        }
        ptr=ptr->next;
    }
    return counter;
}

/*6*/
size_t _num_free_bytes(){ //no need for listMax right??
    size_t to_return=0;
    MetaData* curr_ptr=list.head;
    while(curr_ptr){
        if(curr_ptr->is_free){
            to_return+=curr_ptr->size;
        }
        curr_ptr=curr_ptr->next;
    }
//    MetaData* curr_max_ptr=listMax.head;
//    while(curr_max_ptr){
//        if(curr_max_ptr->is_free){
//            to_return+=curr_max_ptr->size;
//        }
//        curr_max_ptr=curr_max_ptr->next;
//    }
    return to_return;
}

/*7*/
size_t _num_allocated_blocks(){ //?????
//    return (list.size + listMax.size);
    size_t size=0;
    MetaData* curr_ptr=list.head;
    while(curr_ptr){
        size++;
        curr_ptr=curr_ptr->next;
    }
    curr_ptr=listMax.head;
    while(curr_ptr){
        size++;
        curr_ptr=curr_ptr->next;
    }
    return size;
}

/*8*/
size_t _num_allocated_bytes(){
    size_t num=0;
    MetaData* curr_ptr=list.head;
    while(curr_ptr){
        num+=curr_ptr->size;
        curr_ptr=curr_ptr->next;
    }
    curr_ptr=listMax.head;
    while(curr_ptr){
        num+=curr_ptr->size;
        curr_ptr=curr_ptr->next;
    }
    return num;
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