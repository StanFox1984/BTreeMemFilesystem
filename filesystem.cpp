/*
  Sample filesystem in user space and in memory using BTree and custom memory allocator.
  Lisovskiy Stanislav Stanislav.Lisovskiy@gmail.com
*/


#define FUSE_USE_VERSION 30

#include <config.h>

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "btree.h"
#include "mem_allocator.h"
#include "coder.h"

CharBTree<5> *tree = NULL;
MemoryAllocator2 *mem = NULL;
Coder *coder = NULL;

#define MAX_CHANGES 50
#define BUFSIZE 50000000

int changes_counter = MAX_CHANGES;

unsigned char buffer[BUFSIZE];

static void check_changes(CharBTree<5> *tree, MemoryAllocator2 *mem)
{
    changes_counter--;
    if(changes_counter==0)
    {
        mem->syncToDisk("/memory_file_backup");
        tree->syncToDisk("/btree_file_backup");
        changes_counter = MAX_CHANGES;
    }
    return;
}

static int filesystem_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if (strcmp(path, "/memory") == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = sizeof(buffer);
    }
    else {
        char *_str = NULL;
        CharBTree<5>::CharBNode *node = tree->FindNode(path);
        tree->Print();
        if(!node)
            return -ENOENT;
        _str = node->value;
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = node->size;
    }

    return res;
}

static int filesystem_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                              off_t offset, struct fuse_file_info *fi,
                              enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
    {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, "memory", NULL, 0, 0);
    vector< CharBTree<5>::CharBNode * > vec;
    tree->TraverseTree(vec, NULL, false);
    tree->Print();
    for(int i=0; i<vec.size(); i++)
        filler(buf, vec[i]->data+1, NULL, 0, 0);

    return 0;
}

static int filesystem_open(const char *path, struct fuse_file_info *fi)
{
    if (strcmp(path, "/memory") == 0) {
        return 0;
    }
    CharBTree<5>::CharBNode *node = tree->FindNode(path);
    if(!node)
    {
        char *_path = (char*)mem->Allocate(strlen(path)+1);
        memcpy(_path, path, strlen(path)+1);
        char *str = (char*)mem->Allocate(2);
        strcpy(str,"");
        tree->AddNode(_path, str);
    }
    mem->syncToDisk("/memory_file");
    tree->syncToDisk("/btree_file");
    check_changes(tree, mem);
    return 0;
}

static int filesystem_read(const char *path, char *buf, size_t size, off_t offset,
                           struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    char *_str = NULL;
    if (strcmp(path, "/memory") == 0) {
        if (offset < sizeof(buffer)) {
            if (offset + size > sizeof(buffer))
                size = sizeof(buffer) - offset;
            memcpy(buf, buffer + offset, size);
        } else
            size = 0;
        return size;
    }
    CharBTree<5>::CharBNode *node = tree->FindNode(path);
    if(!node)
    {
        return -ENOENT;
    }
    _str = node->value;
    len = node->size;
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, _str + offset, size);
        coder->decode(buf,buf,size);
    }
    else
    {
        size = 0;
    }

    return size;
}

static int filesystem_write(const char *path, const char *buf, size_t size,
                            off_t offset, struct fuse_file_info *fi)
{
    CharBTree<5>::CharBNode *node = tree->FindNode(path);
    if(!node)
    {
        return -ENOENT;
    }
    char *new_val = node->value;
    if((offset + size) > node->size) {
        new_val = (char*)mem->Allocate((offset+size+1)*2);
        memcpy(new_val, node->value, node->size);
        if(node->value)
        {
            mem->Free(node->value);
        }
        node->value = new_val;
        node->size = (offset + size + 1)*2;
    }
    memcpy(&new_val[offset], buf, size);
    new_val[offset+size+1] = 0;
    coder->encode(&new_val[offset], &new_val[offset], size+1);
    mem->syncToDisk("/memory_file");
    tree->syncToDisk("/btree_file");
    check_changes(tree, mem);
    return size;
}

static int filesystem_unlink(const char *path)
{
    CharBTree<5>::CharBNode *node = tree->FindNode(path);
    if(!node) return -ENOENT;
    tree->RemoveNode(node, false);
    mem->syncToDisk("/memory_file");
    tree->syncToDisk("/btree_file");
    check_changes(tree, mem);
    return 0;
}

static int filesystem_mknod(const char *path, mode_t mode, dev_t rdev)
{
    if (strcmp(path, "/memory") == 0) {
        return 0;
    }
    CharBTree<5>::CharBNode *node = tree->FindNode(path);
    if(!node)
    {
        char *_path = (char*)mem->Allocate(strlen(path)+1);
        memcpy(_path, path, strlen(path)+1);
        char *str = (char*)mem->Allocate(2);
        strcpy(str,"");
        tree->AddNode(_path, str);
    }
    mem->syncToDisk("/memory_file");
    tree->syncToDisk("/btree_file");
    check_changes(tree, mem);
    return 0;
}
static int filesystem_chown(const char *path, uid_t uid, gid_t gid)
{
    return 0;
}

static int filesystem_chmod(const char *path, mode_t mode)
{
    return 0;
}

static struct fuse_operations filesystem_oper;

int main(int argc, char *argv[])
{
    char b[30];
    mem = new MemoryAllocator2(buffer, sizeof(buffer), 32, 8192);
    mem->PrintBlocksUsage();
    mem->syncFromDisk("/memory_file");
    tree = new CharBTree<5>(mem);
    tree->syncFromDisk("/btree_file");
    coder = new Coder("pass");
    coder->encode("Hello", b, 5);
    coder->decode(b,b,5);
    printf(b);
//    char *str = mem->Allocate(20);
    mem->PrintBlocksUsage();
    filesystem_oper.getattr  = filesystem_getattr;
    filesystem_oper.readdir  = filesystem_readdir;
    filesystem_oper.open     = filesystem_open;
    filesystem_oper.read     = filesystem_read;
    filesystem_oper.write    = filesystem_write;
    filesystem_oper.mknod    = filesystem_mknod;
    filesystem_oper.unlink   = filesystem_unlink;
    filesystem_oper.chown    = filesystem_chown;
    filesystem_oper.chmod    = filesystem_chmod;
    return fuse_main(argc, argv, &filesystem_oper, NULL);
}
