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

int max_backup_changes = MAX_CHANGES;
int max_write_changes = MAX_CHANGES;

int backup_changes_counter = MAX_CHANGES;
int changes_counter = MAX_CHANGES;

unsigned char buffer[BUFSIZE];

char *memory_file = "/memory_file";
char *btree_file = "/btree_file";
char *backup_memory_file = NULL;
char *backup_btree_file = NULL;

CharBTree<5>::CharBNode *root_node = NULL;

CharBTree<5>::CharBNode *remove_poison_node = (CharBTree<5>::CharBNode *)0xffffffff;

bool flat_view = false;

static void sync_changes(CharBTree<5> *tree, MemoryAllocator2 *mem)
{
    mem->syncToDisk(memory_file);
    tree->syncToDisk(btree_file);
    return;
}

static void check_init_backup_names(void)
{
    if(backup_memory_file == NULL || backup_btree_file == NULL)
    {
        backup_memory_file = (char*)malloc(strlen(memory_file)+strlen("_backup")+1);
        backup_btree_file = (char*)malloc(strlen(btree_file)+strlen("_backup")+1);
        strcpy(backup_memory_file, memory_file);
        strcat(backup_memory_file, "_backup");
        strcpy(backup_btree_file, btree_file);
        strcat(backup_btree_file, "_backup");
    }
    return;
}

static void sync_backup_changes(CharBTree<5> *tree, MemoryAllocator2 *mem)
{
    check_init_backup_names();
    mem->syncToDisk(backup_memory_file);
    tree->syncToDisk(backup_btree_file);
    return;
}

static void check_changes_for_backup(CharBTree<5> *tree, MemoryAllocator2 *mem)
{
    backup_changes_counter--;
    if(backup_changes_counter==0)
    {
        sync_backup_changes(tree, mem);
        backup_changes_counter = max_backup_changes;
    }
    return;
}

static void check_write_changes(CharBTree<5> *tree, MemoryAllocator2 *mem)
{
    changes_counter--;
    if(changes_counter==0)
    {
        sync_changes(tree, mem);
        changes_counter = max_write_changes;
    }
    return;
}


static int filesystem_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 1;
    }
    else if (strstr(path, "/memory") != NULL) {
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
	if(node->attr.is_dir)
	{
	    stbuf->st_mode = S_IFDIR | 0755;
	}
        stbuf->st_nlink = node->filesystem_nodes->size();
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

    CharBTree<5>::CharBNode *node = tree->FindNode(path);

    if (strcmp(path, "/") != 0)
    {
        node = tree->FindNode(path);
    }
    else
    {
        node = root_node;
    }

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
#ifdef MEMORY_FILE
    filler(buf, "memory", NULL, 0, 0);
#endif
    vector< CharBTree<5>::CharBNode * > vec;
    if(flat_view)
    {
	tree->TraverseTree(vec, node, false);
        tree->Print();
	for(int i=0; i<vec.size(); i++)
    	   filler(buf, vec[i]->data+1, NULL, 0, 0);
    }
    else
    {
	for(int i=0;i<node->filesystem_nodes->size();i++)
	{
	    if((*(node->filesystem_nodes))[i] != remove_poison_node)
	    {
    		DEBUG("Displaying path %s node %s\n", path, ((*(node->filesystem_nodes))[i])->data);
		char *s = strrchr(((*(node->filesystem_nodes))[i])->data, '/');
		if(s)
		{
		    filler(buf, s+1, NULL, 0, 0);
		}
	    }
	}
    }

    return 0;
}

static int filesystem_mkdir(const char *path, mode_t mode)
{
    if (strcmp(path, "/memory") == 0) {
        return 0;
    }
    CharBTree<5>::CharBNode *node = tree->FindNode(path);
    int last_slash = strrchr(path, '/')-path;
    char *parent_path = (char*)mem->Allocate(last_slash+1);
    strncpy(parent_path, path, last_slash);
    parent_path[last_slash] = '\0';
    CharBTree<5>::CharBNode *parent_node = tree->FindNode(parent_path);
    if(parent_node == NULL)
    {
        DEBUG("Could not find parent for node path %s, considering root\n", path);
	parent_node = root_node;
    }
    mem->Free(parent_path);
    if(!node)
    {
        char *_path = (char*)mem->Allocate(strlen(path)+1);
        memcpy(_path, path, strlen(path)+1);
        char *str = (char*)mem->Allocate(2);
        strcpy(str,"");
        node = tree->AddNode(_path, str);
	node->attr.is_dir = true;
	node->filesystem_parent = parent_node;
	bool added = false;
	for(int i=0;i<parent_node->filesystem_nodes->size();i++)
	{
	    if((*(parent_node->filesystem_nodes))[i] == remove_poison_node)
	    {
		(*parent_node->filesystem_nodes)[i] = node;
		added = true;
		break;
	    }
	}
	if(!added)
    	    parent_node->filesystem_nodes->push_back(node);
    }
    check_write_changes(tree, mem);
    check_changes_for_backup(tree, mem);
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
    check_changes_for_backup(tree, mem);
    check_write_changes(tree, mem);
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
        new_val = (char*)mem->Allocate((offset+size));
        memcpy(new_val, node->value, node->size);
        if(node->value)
        {
            mem->Free(node->value);
        }
        node->value = new_val;
        node->size = (offset + size);
    }
    memcpy(&new_val[offset], buf, size);
    coder->encode(&new_val[offset], &new_val[offset], size+1);
    check_write_changes(tree, mem);
    check_changes_for_backup(tree, mem);
    return size;
}

static int filesystem_unlink(const char *path)
{
    CharBTree<5>::CharBNode *node = tree->FindNode(path);
    if(!node) return -ENOENT;
    int last_slash = strrchr(path, '/') - path;
    char *parent_path = (char*)mem->Allocate(last_slash+1);
    strncpy(parent_path, path, last_slash);
    parent_path[last_slash] = '\0';
    CharBTree<5>::CharBNode *parent_node = tree->FindNode(parent_path);
    if(parent_node == NULL)
    {
        DEBUG("Could not find parent for node path %s, considering root\n", path);
	parent_node = root_node;
    }
    mem->Free(parent_path);
    for(int i=0;i<parent_node->filesystem_nodes->size();i++)
    {
	if((*(parent_node->filesystem_nodes))[i] == node)
	{
	    (*(parent_node->filesystem_nodes))[i] = remove_poison_node;
	    break;
	}
    }
    tree->RemoveNode(node, false);
    check_write_changes(tree, mem);
    check_changes_for_backup(tree, mem);
    return 0;
}

static int filesystem_rmdir(const char *path)
{
    CharBTree<5>::CharBNode *node = tree->FindNode(path);
    if(!node) return -ENOENT;
    int last_slash = strrchr(path, '/') - path;
    char *parent_path = (char*)mem->Allocate(last_slash+1);
    strncpy(parent_path, path, last_slash);
    parent_path[last_slash] = '\0';
    CharBTree<5>::CharBNode *parent_node = tree->FindNode(parent_path);
    if(parent_node == NULL)
    {
        DEBUG("Could not find parent for node path %s, considering root\n", path);
	parent_node = root_node;
    }
    mem->Free(parent_path);
    for(int i=0;i<parent_node->filesystem_nodes->size();i++)
    {
	if((*(parent_node->filesystem_nodes))[i] == node)
	{
	    (*(parent_node->filesystem_nodes))[i] = remove_poison_node;
	    break;
	}
    }
    tree->RemoveNode(node, false);
    check_write_changes(tree, mem);
    check_changes_for_backup(tree, mem);
    return 0;
}

static int filesystem_mknod(const char *path, mode_t mode, dev_t rdev)
{
    if (strstr(path, "/memory") != NULL) {
        return 0;
    }
    CharBTree<5>::CharBNode *node = tree->FindNode(path);
    if(!node)
    {
        int last_slash = strrchr(path, '/') - path;
	char *parent_path = (char*)mem->Allocate(last_slash+1);
        strncpy(parent_path, path, last_slash);
	parent_path[last_slash] = '\0';
        CharBTree<5>::CharBNode *parent_node = tree->FindNode(parent_path);
	if(parent_node == NULL)
        {
            DEBUG("Could not find parent for node path %s, considering root\n", path);
	    parent_node = root_node;
        }
	mem->Free(parent_path);
        char *_path = (char*)mem->Allocate(strlen(path)+1);
        memcpy(_path, path, strlen(path)+1);
        char *str = (char*)mem->Allocate(2);
        strcpy(str,"");
        CharBTree<5>::CharBNode *node = tree->AddNode(_path, str);
	node->attr.is_dir = false;
	node->filesystem_parent = parent_node;
	bool added = false;
	for(int i=0;i<parent_node->filesystem_nodes->size();i++)
	{
	    if((*(parent_node->filesystem_nodes))[i] == remove_poison_node)
	    {
		(*(parent_node->filesystem_nodes))[i] = node;
		added = true;
		break;
	    }
	}
	if(!added)
    	    parent_node->filesystem_nodes->push_back(node);
	DEBUG("Added node %s to parent node %s\n", node->data, parent_node->data);
    }
    check_write_changes(tree, mem);
    check_changes_for_backup(tree, mem);
    return 0;
}

static int filesystem_fsync(const char *path, int isdatasync,
                            struct fuse_file_info *fi)
{
    sync_changes(tree, mem);
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

extern int opterr;



#define FILESYSTEM_NUMBUCKETS 5

int main(int argc, char *argv[])
{
    int argc_fuse = 2;
    int opt;
    opterr = 0;
    int num_blocks_each_size = 32;
    int start_block_size = 8192;
    char *pass = "";
    char **argv_fuse = new char *[argc+1];
    argv_fuse[0] = argv[0];
    if(argc < 2)
    {
        printf("No mount point specified!\n");
        return -1;
    }
    argv_fuse[1] = argv[1];
    argv_fuse[2] = "-d";
    int n = 2;

    while (n < argc) {

        opt = getopt(argc, argv, "m:t:e:s:p:b:w:");

        if(opt != -1)
        {
            switch (opt) {
            case 'm':
                memory_file = optarg;
                break;
            case 't':
                btree_file = optarg;
                break;
            case 'e':
                num_blocks_each_size = atoi(optarg);
                break;
            case 'w':
                max_write_changes = atoi(optarg);
                break;
            case 'b':
                max_backup_changes = atoi(optarg);
                break;
            case 's':
                start_block_size = atoi(optarg);
                break;
            case 'p':
                pass = optarg;
                break;
            default:
                char *s = new char[3];
                s[0] = '-';
                s[1] = optopt;
                s[2] = '\0';
                argv_fuse[argc_fuse] = s;
                argc_fuse++;
            }
        }
        n++;
    }

    printf("Memory file %s btree_file %s num_blocks_each_size %d start %d pass %s max_write_changes %d max_backup_changes %d\n",
           memory_file, btree_file, num_blocks_each_size, start_block_size, pass, max_write_changes, max_backup_changes);

    printf("fuse_argc %d fuse args:\n", argc_fuse);
    for(int i=0; i<argc_fuse; i++)
    {
        printf("%s\n", argv_fuse[i]);
    }
    mem = new MemoryAllocator2(buffer, sizeof(buffer), num_blocks_each_size, start_block_size);
    coder = new Coder(pass);
    tree = new CharBTree<FILESYSTEM_NUMBUCKETS>(mem, coder);
    mem->PrintBlocksUsage();
    if(mem->syncFromDisk(memory_file) != 0)
    {
        check_init_backup_names();
	delete mem;
	printf("Could not open memory file %s, trying _backup..\n", memory_file);
        mem = new MemoryAllocator2(buffer, sizeof(buffer), num_blocks_each_size, start_block_size);
	delete tree;
	printf("Could not open btree file %s, trying _backup..\n", btree_file);
        tree = new CharBTree<FILESYSTEM_NUMBUCKETS>(mem, coder);
	if(tree->syncFromDisk(backup_btree_file) != 0)
	{
    	    printf("Could not open backup btree file %s\n", backup_btree_file);
	    delete tree;
	    tree = new CharBTree<FILESYSTEM_NUMBUCKETS>(mem, coder);
	}
	if(mem->syncFromDisk(backup_memory_file) != 0)
	{
    	    printf("Could not open backup memory file %s\n", backup_memory_file);
	    delete mem;
	    mem = new MemoryAllocator2(buffer, sizeof(buffer), num_blocks_each_size, start_block_size);
	    delete tree;
	    tree = new CharBTree<FILESYSTEM_NUMBUCKETS>(mem, coder);
	}

    }
    if(tree->syncFromDisk(btree_file) != 0)
    {
        check_init_backup_names();
	delete tree;
	printf("Could not open btree file %s, trying _backup..\n", btree_file);
        tree = new CharBTree<FILESYSTEM_NUMBUCKETS>(mem, coder);
	if(tree->syncFromDisk(backup_btree_file) != 0)
	{
    	    printf("Could not open backup btree file %s\n", backup_btree_file);
	    delete tree;
	    tree = new CharBTree<FILESYSTEM_NUMBUCKETS>(mem, coder);
	}
    }

    root_node = tree->getRoot();

    if(!root_node)
    {
	char *root_path =(char *) mem->Allocate(strlen("/")+1);
	root_path[0] = '/';
	root_path[1] = '\0';
	tree->AddNode(root_path, "");
	root_node = tree->getRoot();
    }

    filesystem_oper.getattr  = filesystem_getattr;
    filesystem_oper.readdir  = filesystem_readdir;
    filesystem_oper.mkdir    = filesystem_mkdir;
    filesystem_oper.rmdir    = filesystem_rmdir;
    filesystem_oper.open     = filesystem_open;
    filesystem_oper.read     = filesystem_read;
    filesystem_oper.write    = filesystem_write;
    filesystem_oper.mknod    = filesystem_mknod;
    filesystem_oper.unlink   = filesystem_unlink;
    filesystem_oper.chown    = filesystem_chown;
    filesystem_oper.chmod    = filesystem_chmod;
    filesystem_oper.fsync    = filesystem_fsync;

    return fuse_main(argc_fuse, argv_fuse, &filesystem_oper, NULL);
}
