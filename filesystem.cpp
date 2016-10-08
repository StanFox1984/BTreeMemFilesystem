/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 *
 * hello.c - minimal FUSE example featuring fuse_main usage
 *
 * \section section_compile compiling this example
 *
 * gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
 *
 * \section section_usage usage
 \verbatim
 % mkdir mnt
 % ./hello mnt        # program will vanish into the background
 % ls -la mnt
   total 4
   drwxr-xr-x 2 root root      0 Jan  1  1970 ./
   drwxrwx--- 1 root vboxsf 4096 Jun 16 23:12 ../
   -r--r--r-- 1 root root     13 Jan  1  1970 hello
 % cat mnt/hello
   Hello World!
 % fusermount -u mnt
 \endverbatim
 *
 * \section section_source the complete source
 * \include hello.c
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


static const char *hello_str = "Hello World!\n";
static const char *hello_str2 = "Hello World2!\n";
static const char *hello_path = "/hello";
static const char *hello_path2 = "/hello2";

CharBTree<5> tree;
MemoryAllocator2 *mem = NULL;

unsigned char buffer[5000000];

static int hello_getattr(const char *path, struct stat *stbuf)
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
        char *_hello_str = NULL;
        CharBTree<5>::CharBNode *node = tree.FindNode(path);
        if(!node)
            return -ENOENT;
        _hello_str = node->value;
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(_hello_str);
    }

    return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, "memory", NULL, 0, 0);
    vector< CharBTree<5>::CharBNode * > vec;
    tree.TraverseTree(vec);
    for(int i=0; i<vec.size(); i++)
        filler(buf, vec[i]->data+1, NULL, 0, 0);

    return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
    if (strcmp(path, "/memory") == 0) {
        return 0;
    }
    CharBTree<5>::CharBNode *node = tree.FindNode(path);
    if(!node)
    {
        char *_path = (char*)mem->Allocate(strlen(path)+1);
        memcpy(_path, path, strlen(path)+1);
        char *str = (char*)mem->Allocate(2);
        strcpy(str,"\n");
        tree.AddNode(_path, str);
    }
    return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    char *_hello_str = NULL;
    if (strcmp(path, "/memory") == 0) {
        if (offset < sizeof(buffer)) {
            if (offset + size > sizeof(buffer))
                size = sizeof(buffer) - offset;
            memcpy(buf, buffer + offset, size);
        } else
            size = 0;
        return size;
    }
    CharBTree<5>::CharBNode *node = tree.FindNode(path);
    if(!node)
        return -ENOENT;
    _hello_str = node->value;
    len = strlen(_hello_str);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, _hello_str + offset, size);
    } else
        size = 0;

    return size;
}

static int hello_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi)
{
    CharBTree<5>::CharBNode *node = tree.FindNode(path);
    if(!node)
        return -ENOENT;
    char *new_val = NULL;
    if(offset + size > strlen(node->value)+1) {
        new_val = (char*)mem->Allocate(offset+size+1);
        memcpy(new_val, node->value, strlen(node->value)+1);
    }
    memcpy(&new_val[offset], buf, size);
    new_val[offset+size+1] = 0;
    if(node->value)
        mem->Free(node->value);
    node->value = new_val;

    return size;
}

static int hello_unlink(const char *path)
{
    CharBTree<5>::CharBNode *node = tree.FindNode(path);
    if(!node) return -ENOENT;
    tree.RemoveNode(node, false);
    return 0;
}

static int hello_mknod(const char *path, mode_t mode, dev_t rdev)
{
    if (strcmp(path, "/memory") == 0) {
        return 0;
    }
    CharBTree<5>::CharBNode *node = tree.FindNode(path);
    if(!node)
    {
        char *_path = (char*)mem->Allocate(strlen(path)+1);
        memcpy(_path, path, strlen(path)+1);
        char *str = (char*)mem->Allocate(2);
        strcpy(str,"\n");
        tree.AddNode(_path, str);
    }
    return 0;
}
static int hello_chown(const char *path, uid_t uid, gid_t gid)
{
    return 0;
}

static int hello_chmod(const char *path, mode_t mode)
{
    return 0;
}

static struct fuse_operations hello_oper;

int main(int argc, char *argv[])
{
    mem = new MemoryAllocator2(buffer, sizeof(buffer), 16, 32);
    char *str = mem->Allocate(20);
    mem->PrintBlocksUsage();
    strcpy(str, "Hello World!");
    tree.AddNode(hello_path2, hello_str2);
    tree.AddNode(hello_path, str);
    tree.Print();
    CharBTree<5>::CharBNode *node = tree.FindNode(hello_path);
    tree.RemoveNode(node, false);
    node = tree.FindNode(hello_path2);
    tree.RemoveNode(node);
    printf("sdsdsd %s %s\n",node->data, node->value);
    tree.Print();
    hello_oper.getattr    = hello_getattr;
    hello_oper.readdir    = hello_readdir;
    hello_oper.open        = hello_open;
    hello_oper.read        = hello_read;
    hello_oper.write    = hello_write;
    hello_oper.mknod    = hello_mknod;
    hello_oper.unlink    = hello_unlink;
    hello_oper.chown    = hello_chown;
    hello_oper.chmod    = hello_chmod;
    return fuse_main(argc, argv, &hello_oper, NULL);
}
