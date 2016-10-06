#ifndef MEM_ALLOCATOR_H
#define MEM_ALLOCATOR_H

#include <stdio.h>
#include <list>
#include <vector>
#include <stdlib.h>

using namespace std;



struct MemPtr
{
    MemPtr *next;
    MemPtr *prev;
    void *address;
    bool allocated;
    unsigned long size;
    unsigned long sane_magic;
};

int num_allocs = 0;

int ceil_log2(unsigned long x)
{
  bool power_of_two = (x & x-1) == 0 ? true : false;
  int k;
  int t;
  /*
  0000        0
  0001        1
  0010        2
  0011        2
  0100        3
  0101        3
  0110        3
  0111        3
  1000        4
  ...
  */
  unsigned char highest_set[] = { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 } ;
  
  if( x == 0 )
  {
    return 0;
  }
  if(x & 0xffff0000)
  {
      if(x & 0xff000000)
      {
     if(x & 0xf0000000)
     {
        k = highest_set[ (x & 0xf0000000) >> 28 ] + 28;
     }
     else
     {
        k = highest_set[ (x & 0x0f000000) >> 24 ] + 24;
     }
      }
      else
      {
     if(x & 0x00f00000)
     {
        k = highest_set[ (x & 0x00f00000) >> 20 ] + 20;
     }
     else
     {
        k = highest_set[ (x & 0x000f0000) >> 16 ] + 16;
     }
      }
  }
  else
  {
      if(x & 0x0000ff00)
      {
         if(x & 0x0000f000)
     {
        k = highest_set[ (x & 0x0000f000) >> 12 ] + 12;
     }
     else
     {
        k = highest_set[ (x & 0x00000f00) >> 8 ] + 8;
     }
      }
      else
      {
     if(x & 0x000000f0)
     {
        k = highest_set[ (x & 0x000000f0) >> 4 ] + 4;
     }
     else
     {
        k = highest_set[ x & 0x0000000f ];
     }
      }
  }

  if(power_of_two) return k-1;

  return k;
}

class DefaultAllocator
{
    public:
    virtual void *Allocate(int size)
    {
    return malloc(size);
    }
    virtual void Free(void *ptr)
    {
    free(ptr);
    }
};

class MemoryAllocator2:public DefaultAllocator
{
public:
    unsigned long lowest_address;
    unsigned long highest_address;
    int skip;
    vector<int>   free_blocks_num;
    MemoryAllocator2(void *ptr, int size, int each=16384, unsigned long start = 128)
    {
        unsigned long remain = size;
        unsigned long alloc = 0;
        int n=0, i=0;
        skip = ceil_log2(start);
        free_blocks.push_back(NULL);
        free_blocks_num.push_back(0);
        lowest_address = (unsigned long)ptr;
        highest_address = (unsigned long)ptr + remain;
        if(!ptr)
        {
            throw "Bad ptr!";
        }
        while( (remain > start) && start )
        {
            for( i=0;i<each;i++)
            {
                MemPtr *block = (MemPtr*)malloc(sizeof(MemPtr));
                block->address = (void*)((unsigned long)ptr + alloc);
                block->prev = NULL;
                block->next = free_blocks[n];
                block->size = start;
                block->sane_magic = (unsigned long)block;
                block->allocated = false;
                free_blocks[n] = block;
                remain -= start;
                alloc += start;
                free_blocks_num[n]++;
                if(remain < start )
                    break;
            }
            printf("remain %d alloc %d start %d last_each %d free_blocks %d\n", remain, alloc, start, i,free_blocks_num[n]);
            if(remain < start )
                break;
            start = start<<1;
            free_blocks.push_back(NULL);
            free_blocks_num.push_back(0);
            n++;
        }
    }
    bool is_sane(MemPtr *block)
    {
        bool block_correct = (block->sane_magic == (unsigned long)block);
        bool addr_correct = (((unsigned long)block->address <= highest_address) && ((unsigned long)block->address>=lowest_address));
        return block_correct && addr_correct;
    }
    bool AddBlocksNextSize(int n)
    {
        printf("Ran out of pool %d!\n", n);
        int initial_n = n;
        while((!free_blocks[n]) && (n<free_blocks.size())) n++;
        if(n >= free_blocks.size())
        return false;
        MemPtr *block = (MemPtr*)malloc(sizeof(MemPtr));
        block->size    = free_blocks[n]->size>>1;
        block->address = free_blocks[n]->address;
        block->next = free_blocks[initial_n];
        block->allocated = false;
        block->prev = NULL;
        block->sane_magic = (unsigned long)block;
        free_blocks[initial_n] = block;
        block = free_blocks[n];
        free_blocks[n] = free_blocks[n]->next;
        free_blocks_num[n]--;
        block->address = (void*)((unsigned long)free_blocks[initial_n]->address + free_blocks[initial_n]->size);
        block->next = free_blocks[initial_n];
        block->allocated = false;
        block->prev = NULL;
        block->size = free_blocks[initial_n]->size;
        free_blocks[initial_n] = block;
        free_blocks_num[initial_n]++;
        return true;
    }
    void *Allocate(int size)
    {
        int n;
        if(size == last_cached_size)
            n = last_n;
        else { n  = ceil_log2(size) - skip;
        last_cached_size = size;
        last_n = n; }
        if(n < 0) n = 0;
        if(n >= free_blocks.size())
            return NULL;

        if(!free_blocks[n])
            if(!AddBlocksNextSize(n))
            return NULL;
        if(!is_sane(free_blocks[n]))
            return NULL;
        *((unsigned long*)(free_blocks[n]->address)) = (unsigned long)(free_blocks[n]);
        void *ptr = (void*)(((unsigned long)free_blocks[n]->address) + sizeof(unsigned long));
        free_blocks[n]->allocated = true;
        free_blocks[n] = free_blocks[n]->next;
        free_blocks_num[n]--;
        return ptr;
    }
    void Free(void *ptr)
    {
        MemPtr *block = (MemPtr*)(*((unsigned long*)((unsigned long)ptr-sizeof(unsigned long))));
        if(!is_sane(block))
            throw;
        if(!block->allocated)
            throw;
        int n;
        if(block->size == last_cached_size)
            n = last_n;
        else { n  = ceil_log2(block->size) - skip;
        last_cached_size = block->size;
        last_n = n; }
        if(n < 0) throw;
        if(n >= free_blocks.size())
            throw;
        block->next = free_blocks[n];
        block->allocated = false;
        free_blocks[n] = block;
        free_blocks_num[n]++;
    }
        void PrintBlocksUsage(void)
    {
        for(int i=0;i<free_blocks_num.size();i++)
        {
        printf("Size %d Free blocks: %d\n", 1<<(i+skip), free_blocks_num[i]);
        }
    }
protected:
    vector< MemPtr *>   free_blocks;
    int last_cached_size;
    int last_n;
};

#endif