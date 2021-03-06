#ifndef MEM_ALLOCATOR_H
#define MEM_ALLOCATOR_H

#include <stdio.h>
#include <list>
#include <vector>
#include <stdlib.h>

using namespace std;

#define DEBUG printf
//#define DEBUG(...)

struct MemPtr
{
    long long    sane_magic1;
    MemPtr *next;
    MemPtr *prev;
    void *address;
    bool allocated;
    unsigned long size;
    long long    sane_magic2;
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
    virtual void *GetLowestAddress(void)
    {
        return NULL;
    }
    virtual void *GetHighestAddress(void)
    {
        return (void*)0xffffffff;
    }
};

class MemoryAllocator2:public DefaultAllocator
{
public:
    unsigned long lowest_address;
    unsigned long highest_address;
    int skip;
    vector<int>   free_blocks_num;
    vector<int>   alloc_blocks_num;
    int           last_block_index;

    void syncToDisk(const char *filename)
    {
        FILE *fout = NULL;
        fout = fopen(filename, "w+b");
        if(!fout)
        {
            throw "Could not write to disk!";
        }
        DEBUG("Syncing %x-%x to disk to file %s\n", lowest_address, lowest_address + (highest_address-lowest_address)/2, filename);
        fwrite((void*)lowest_address, (highest_address-lowest_address)/2, 1, fout);
        int num_alloc_blocks = 0;
        for(int i=0; i<alloc_blocks_num.size(); i++)
            num_alloc_blocks += alloc_blocks_num[i];
        fwrite((void*)&num_alloc_blocks, sizeof(int), 1, fout);
        DEBUG("Wrote num_alloc_blocks %d\n", num_alloc_blocks);
        for(int i=0; i<alloc_blocks.size(); i++)
        {
            MemPtr *block = alloc_blocks[i];
            for(int j=0; j<alloc_blocks_num[i]; j++)
            {
                int size = 0;
                void *address = NULL;
                size = block->size;
                address = block->address;
                fwrite((void*)&address, sizeof(void*), 1, fout);
                fwrite((void*)&size, sizeof(int), 1, fout);
                block = block->next;
                DEBUG("Wrote block %x size %d\n", address, size);
            }
        }
        fwrite((void*)&last_block_index, sizeof(int), 1, fout);
        fclose(fout);
        return;
    }
    int syncFromDisk(const char *filename)
    {
        FILE *fin = NULL;
        fin = fopen(filename, "r+b");
        if(!fin)
        {
            DEBUG("Could not open file %s\n", filename);
            return -1;
        }
        DEBUG("Syncing %x-%x from disk file %s\n", lowest_address, lowest_address + (highest_address-lowest_address)/2, filename);
        fread((void*)lowest_address, (highest_address-lowest_address)/2, 1, fin);
        int num_alloc_blocks = 0;
        fread((void*)&num_alloc_blocks, sizeof(int), 1, fin);
        DEBUG("Read num_alloc_blocks %d\n", num_alloc_blocks);
        for(int i=0; i<num_alloc_blocks; i++)
        {
            int size = 0;
            void *address = NULL;
            fread((void*)&address, sizeof(void*), 1, fin);
            fread((void*)&size, sizeof(int), 1, fin);
            DEBUG("Read block %x size %d\n", address, size);
            void *ptr = AllocateSpecificAddress(address, size);
            MemPtr *block = GetBlockFromPtr(ptr);
            if(is_sane(block))
            {
                DEBUG("Sanity check passed for block %x ptr %x\n", block, ptr);
            }
        }
        fread((void*)&last_block_index, sizeof(int), 1, fin);
        fclose(fin);
        return 0;
    }
    MemoryAllocator2(void *ptr, int size, int each=16384, unsigned long start = 128)
    {
        unsigned long remain = size/2;
        unsigned long alloc = 0;
        int n=0, i=0;
        int block_index = 0;
        skip = ceil_log2(start);
        free_blocks.push_back(NULL);
        free_blocks_num.push_back(0);
        alloc_blocks.push_back(NULL);
        alloc_blocks_num.push_back(0);
        lowest_address = (unsigned long)ptr;
        highest_address = (unsigned long)ptr + size;
        if(!ptr)
        {
            throw "Bad ptr!";
        }
        while( (remain > start) && start && ((alloc+start) < (size/2)) )
        {
            for( i=0; i<each; i++)
            {
                if((alloc+start) >= (size/2)) break;
                unsigned long block_address = (MemPtr*)(ptr+size/2+sizeof(MemPtr)*block_index);
                if(block_address >= highest_address)
                {
                    DEBUG("Wrong block address calculation\n");
                    throw;
                }
                MemPtr *block = (MemPtr*)block_address;
                block->address = (void*)((unsigned long)alloc);
                block->prev = NULL;
                block->next = NULL;
                block->size = start;
                block->sane_magic1 = (unsigned long)block;
                block->sane_magic2 = (unsigned long)block;
                AddBlockToFreeQueue(block, n);
                remain -= start;
                alloc += start;
                block_index++;
                if(remain < start )
                    break;
            }

            DEBUG("remain %d alloc %d start %d last_each %d free_blocks %d\n", remain, alloc, start, i,free_blocks_num[n]);

            if(remain < start )
                break;
            start = start<<1;
            free_blocks.push_back(NULL);
            free_blocks_num.push_back(0);
            alloc_blocks.push_back(NULL);
            alloc_blocks_num.push_back(0);
            n++;
        }
        last_block_index = block_index;
    }
    bool is_sane(MemPtr *block)
    {
        if(block == NULL) {
            DEBUG("block is NULL\n");
            throw;
        }
        bool block_correct = (block->sane_magic1 == (unsigned long)block) && (block->sane_magic2 == (unsigned long)block);
	if(block->allocated)
	    block_correct = block_correct && (*((unsigned long*)(block->address + lowest_address)) == block);
        if(!block_correct) DEBUG("block sane magic is wrong: %x %x should be both %x\n", block->sane_magic1, block->sane_magic2, block);
        bool addr_correct = (((unsigned long)block->address+lowest_address <= highest_address) && ((unsigned long)block->address+lowest_address>=lowest_address));
        if(!addr_correct) DEBUG("block address is wrong: %x should be between %x-%x\n", block->address, lowest_address, highest_address);
        return block_correct && addr_correct;
    }
    bool AddBlocksNextSize(int n)
    {
        /*we ran out of blocks of size n, lets see if we have bigger free blocks */
        /*take bigger one, split it and add to the queue which lacks free blocks*/
        /*TODO: we need in fact to split it more that twice, but depending how much
          bigger it is*/


        int initial_n = n;
        while((!free_blocks[n]) && (n<free_blocks.size())) n++;
        if(n >= free_blocks.size())
        {
            DEBUG("No bigger blocks available!\n");
            /* ops looks like no bigger free blocks were available */
            return false;
        }
        DEBUG("Found block n %d(size %d), start splitting\n", n, 1<<(n+skip));
        int num_splits = 1<<(n - initial_n);
        int i=0;
        for(; i<num_splits; i++)
        {
            /*we found bigger free block, lets create a new descriptor and initialize */
            unsigned long block_address = (lowest_address+((highest_address-lowest_address)/2)+sizeof(MemPtr)*last_block_index);
            DEBUG("Created new block descriptor %x for n %d\n", block_address, initial_n);
            if(block_address >= highest_address) return false;
            MemPtr *block = (MemPtr*)block_address;
            block_address = (unsigned long)free_blocks[n]->address + (1<<(initial_n+skip))*i;
            if((block_address+(1<<(initial_n+skip))) >= ((highest_address - lowest_address)/2))
            {
                DEBUG("Block address goes beyond the correct range!\n");
                throw;
            }
            last_block_index++;
            block->address = block_address;
            block->size    = 1<<(initial_n+skip);
            block->allocated = false;
            block->prev = NULL;
            block->sane_magic1 = (unsigned long)block;
            block->sane_magic2 = (unsigned long)block;
            DEBUG("Split block %x(addr %x) from n %d(size %d) to block %x(addr %x) n %d(size %d)\n", 
            free_blocks[n], free_blocks[n]->address, n, 1<<(n + skip), block, block->address, initial_n, 1<<(initial_n + skip));
            AddBlockToFreeQueue(block, initial_n);
            /*added to the head of the queue*/
        }
        MemPtr *block = free_blocks[n];
        /*shift the queue of bigger blocks as this one is not available anymore*/
        RemoveBlockFromQueue(block, n, free_blocks);
        free_blocks_num[n]--;
        /*add it to the end of initial queue*/
        AddBlockToQueue(block, initial_n, free_blocks);
        free_blocks_num[initial_n]++;
        /*the remaining part of size is a another block, increment the address*/
        unsigned long block_address = ((unsigned long)free_blocks[initial_n]->address + i*(1<<(initial_n+skip)));
        if((block_address+(1<<(initial_n+skip))) >= ( (highest_address - lowest_address)/2))
        {
            DEBUG("Block address goes beyond the correct range!\n");
            throw;
        }
        block->address = (void*)block_address;
        block->allocated = false;
        block->size = 1<<(initial_n+skip);
        block->sane_magic1 = block;
        block->sane_magic2 = block;
        DEBUG("Split block %x(addr %x) from n %d(size %d) to block %x(addr %x) n %d(size %d)\n", 
        block, block->address - i*(1<<(initial_n+skip)), n, 1<<(n + skip), block, block->address, initial_n, 1<<(initial_n + skip));
        /*make it same size, initialize*/
        return true;
    }
    void *GetLowestAddress(void)
    {
        return lowest_address;
    }
    void *GetHighestAddress(void)
    {
        return highest_address;
    }
    void SetBlockToPtr(void *ptr, MemPtr *block)
    {
        (*((unsigned long*)((unsigned long)ptr-sizeof(unsigned long)))) = (unsigned long)block;
    }
    MemPtr *GetBlockFromPtr(void *ptr)
    {
        MemPtr *block = (MemPtr*)(*((unsigned long*)((unsigned long)ptr-sizeof(unsigned long))));
        DEBUG("Allocator: Got block %x from ptr %x\n", block, ptr);
        return block;
    }
    void *Allocate(int _size)
    {
        int n;
        int size = _size + sizeof(unsigned long);
        if(size == last_cached_size)
        {
            n = last_n;
        }
        else {
            n  = ceil_log2(size) - skip;
            last_cached_size = size;
            last_n = n;
        }
        if(n < 0)
        {
            n = 0;
        }
        if(n >= free_blocks.size())
        {
            DEBUG("No free blocks of this size!!!\n");
            return NULL;
        }

        if(!free_blocks[n])
        {
            DEBUG("No free blocks of this size left! Trying to get from bigger ones...\n");
            if(!AddBlocksNextSize(n))
            {
                DEBUG("Could not add from bigger block for n %d\n", n);
                return NULL;
            }
        }
        if(!is_sane(free_blocks[n]))
        {
            DEBUG("n %d queue head block is not sane!\n", n);
            return NULL;
        }
        if(free_blocks[n]->allocated)
        {
            DEBUG("Allocate: Block %x address %x from queue n %d(size %d) is already allocated!\n", free_blocks[n], free_blocks[n]->address, n, 1<<(n+skip));
            throw;
        }
        void *ptr = (void*)(((unsigned long)free_blocks[n]->address) + sizeof(unsigned long)+lowest_address);
        SetBlockToPtr(ptr, free_blocks[n]);
        free_blocks[n]->sane_magic1 = free_blocks[n];
        free_blocks[n]->sane_magic2 = free_blocks[n];
        DEBUG("Setting sane magic %x(at addr %x) for block %x address %x ptr %x\n", free_blocks[n], free_blocks[n]->address+lowest_address, free_blocks[n], free_blocks[n]->address, ptr);
        MemPtr *block = free_blocks[n];
        AddBlockToAllocQueue(block, n);
        DEBUG("Allocator:Returned address %x\n",(unsigned long)ptr);
        return ptr;
    }
    void *AllocateSpecificAddress(void *address, int size)
    {
        int n;
        if(size == last_cached_size)
        {
            n = last_n;
        }
        else {
            n  = ceil_log2(size) - skip;
            last_cached_size = size;
            last_n = n;
        }
        if(n < 0)
        {
            n = 0;
        }
        if(n >= free_blocks.size())
        {
            DEBUG("AllocateSpecificAddress: Ran out of all blocks!\n");
            throw;
        }

        if(!free_blocks[n])
        {
            DEBUG("AllocateSpecificAddress: Ran out of blocks in queue n %d(size %d)!\n", n, 1<<(n+skip));
#ifdef MINDLESS_ALLOC
            if(!AddBlocksNextSize(n))
            {
                return NULL;
            }
#else
            throw;
#endif
        }
        if(!is_sane(free_blocks[n]))
        {
            DEBUG("AllocateSpecificAddress: Block %x address %x from queue n %d(size %d) is insane!\n", free_blocks[n], free_blocks[n]->address, n, 1<<(n+skip));
            throw;
        }
        if(free_blocks[n]->allocated)
        {
            DEBUG("AllocateSpecificAddress: Block %x address %x from queue n %d(size %d) is already allocated!\n", free_blocks[n], free_blocks[n]->address, n, 1<<(n+skip));
            throw;
        }
        MemPtr *block = FindBlockWithAddressInFreeQueue(address, size);
        if(block == NULL)
        {
            DEBUG("Could not find block with address %x size %d queue size %d for n %d(corrupt file or memory conf?)\n", address, size, free_blocks_num[n], n);
            throw;
        }

        void *ptr = (void*)(((unsigned long)block->address) + sizeof(unsigned long)+lowest_address);
        SetBlockToPtr(ptr, block);
        block->sane_magic1 = block;
        block->sane_magic2 = block;
        DEBUG("Setting sane magic %x(at addr %x) for block %x address %x ptr %x\n", block, block->address+lowest_address, block, block->address, ptr);
        AddBlockToAllocQueue(block, n);
        DEBUG("Allocator(specific):Returned address %x\n",(unsigned long)ptr);
        return ptr;
    }
    void Free(void *ptr)
    {
        MemPtr *block = GetBlockFromPtr(ptr);
        if(!is_sane(block))
        {
            DEBUG("Wrong(insane) block %x address %x in ptr %x\n", block, block->address, ptr);
            throw;
        }
        if(!block->allocated)
        {
            DEBUG("Block %x address %x is not allocated(double free?)\n", block, block->address);
            throw;
        }
        int n;
        if(block->size == last_cached_size)
        {
            n = last_n;
        }
        else {
            n  = ceil_log2(block->size) - skip;
            last_cached_size = block->size;
            last_n = n;
        }
        if(n < 0)
            n = 0;
        if(n >= free_blocks.size())
        {
            DEBUG("Wrong(insane) block size n %d(%d)\n", n, 1<<n+skip);
            throw;
        }
        AddBlockToFreeQueue(block, n);
        DEBUG("Allocator: Freed address %x(full %x) n %d(size %d)\n", block->address, ptr, n, 1<<(skip+n));
        return;
    }
    void PrintBlocksUsage(void)
    {
        for(int i=0; i<free_blocks_num.size(); i++)
        {
            DEBUG("Size %d Free blocks: %d\n", 1<<(i+skip), free_blocks_num[i]);
            MemPtr *block = free_blocks[i];
            int u = 0;
            while(true && (block != NULL) )
            {
                DEBUG(" Block %x address %x n %d\n", block, block->address, i, 1<<(i+skip));
                block = block->next;
                if(block == free_blocks[i] || u>16)
                    break;
                u++;
            }
        }
        for(int i=0; i<alloc_blocks_num.size(); i++)
        {
            DEBUG("Size %d Alloc blocks: %d\n", 1<<(i+skip), alloc_blocks_num[i]);

        }
    }
    MemPtr *FindBlockWithAddressInFreeQueue(void *address, int size)
    {
        int n;
        if(size == last_cached_size)
        {
            n = last_n;
        }
        else {
            n  = ceil_log2(size) - skip;
            last_cached_size = size;
            last_n = n;
        }
        if(n < 0)
            throw;
        if(n >= free_blocks.size())
            throw;
        MemPtr *block = free_blocks[n];
        int i=0;
        while(true)
        {
            DEBUG("Checking %x if equal to %x free queue n %d(size %d)\n", block->address, address, n, 1<<(n+skip));
            if(block->address == address)
                break;
            block = block->next;
            if(block == free_blocks[n])
                break;
            i++;
        }
        if(i == free_blocks_num[n] || block->address != address)
            return NULL;
        return block;
    }
    void AddBlockToAllocQueue(MemPtr *block, int n)
    {
        if(!block->allocated)
        {
            DEBUG("Remove block %x address %x from free_queue n %d\n", block, block->address, n);
            RemoveBlockFromQueue(block, n, free_blocks);
        }
        DEBUG("Add block %x address %x to alloc_queue n %d\n", block, block->address, n);
        AddBlockToQueue(block, n, alloc_blocks);
        if(free_blocks_num[n]>0)
            free_blocks_num[n]--;
        alloc_blocks_num[n]++;
        block->allocated = true;
    }
    void AddBlockToFreeQueue(MemPtr *block, int n)
    {
        if(block->allocated)
        {
            DEBUG("Remove block %x address %x from alloc_queue n %d\n", block, block->address, n);
            RemoveBlockFromQueue(block, n, alloc_blocks);
        }
        DEBUG("Add block %x address %x to free_queue n %d\n", block, block->address, n);
        AddBlockToQueue(block, n, free_blocks);
        free_blocks_num[n]++;
        if(alloc_blocks_num[n]>0)
            alloc_blocks_num[n]--;
        block->allocated = false;
    }
    void AddBlockToQueue(MemPtr *block, int n, vector< MemPtr *>  &blocks)
    {
        DEBUG("Added block %x address %x size %d to queue n %d head block %x\n", block, block->address, block->size, n, blocks[n]);
        if(blocks[n] == NULL)
        {
            blocks[n] = block;
            block->prev = block;
            block->next = block;
        }
        else
        {
            blocks[n]->prev->next = block;
            block->prev = blocks[n]->prev;
            blocks[n]->prev = block;
            block->next = blocks[n];
        }
        return;
    }
    void RemoveBlockFromQueue(MemPtr *block, int n, vector< MemPtr *> &blocks)
    {
        DEBUG("Removed block %x address %x size %d from queue n %d head block %x\n", block, block->address, block->size, n, blocks[n]);
        if(blocks[n] == NULL)
        {
            block->next = block;
            block->prev = block;
            return;
        }
        if(blocks[n] == block)
        {
            if(block->next == block)
            {
                blocks[n] = NULL;
            }
            else
            {
                blocks[n] = blocks[n]->next;
            }
        }
        if(block->prev)
            block->prev->next = block->next;
        if(block->next)
            block->next->prev = block->prev;
        return;
    }
protected:
    vector< MemPtr *>   free_blocks;
    vector< MemPtr *>   alloc_blocks;
    int last_cached_size;
    int last_n;
};

#endif