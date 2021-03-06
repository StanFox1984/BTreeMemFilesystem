#ifndef BTREE_H
#define BTREE_H
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <queue>
#include <stack>
#include "coder.h"
#include "mem_allocator.h"

using namespace std;

template<class T,class D, int buckets>
struct BTreeNode
{
    BTreeNode<T, D, buckets>    *nodes[buckets];
    T data;
    D value;

    BTreeNode()
    {
        memset(nodes, NULL, sizeof(void*)*buckets);
    }
    void AddNode(BTreeNode<T, D, buckets> *node)
    {
        int i=0;
        for(; i<buckets; i++)
        {
            if(nodes[i]==NULL)
            {
                if(i>0)
                {
                    if(nodes[i-1]->data < node->data)
                    {
                        nodes[i] = node;
                        break;
                    }
                }
                else
                {
                    nodes[i] = node;
                    break;
                }
            }
            else
            {
                if(i>0)
                {
                    if(nodes[i-1]->data < node->data && node->data<=nodes[i]->data)
                    {
                        nodes[i]->AddNode(node);
                        break;
                    }
                }
                else
                {
                    if(nodes[i]->data >= node->data)
                    {
                        nodes[i]->AddNode(node);
                        break;
                    }
                }
            }
        }
        nodes[i-1].AddNode(node);
    }
};


template<int buckets>
struct BTreeNode<const char *, const char *, buckets>
{
    struct BTreeNodeAttrib     //only put POD types here!
    {
        bool is_dir;
        unsigned long perm_mask;
        unsigned long time_stamp;
    };
    vector<BTreeNode<const char*, const char *, buckets>* >  *filesystem_nodes;
    BTreeNode<const char *, const char *, buckets>    *nodes[buckets];
    const char *data;
    const char *value;
    int        size;
    BTreeNodeAttrib attr;
    BTreeNode<const char *, const char *, buckets>    *parent;
    BTreeNode<const char *, const char *, buckets>    *filesystem_parent;
    void Init(void)
    {
        memset(nodes, NULL, sizeof(void*)*buckets);
        parent = NULL;
        data = NULL;
        value = NULL;
        size = 0;
	filesystem_nodes = new vector<BTreeNode<const char*, const char *, buckets>* >();
    }

    BTreeNode()
    {
        Init();
    }
    static void RemoveNode(BTreeNode<const char *, const char *, buckets> *node)
    {
        BTreeNode<const char *, const char *, buckets> *parent = node->parent;
        if(!parent)
            return;
        for(int i=0; i<buckets; i++)
        {
            if(parent->nodes[i] == node)
            {
                parent->nodes[i] = NULL;
            }
        }
        for(int i=0; i<buckets; i++)
        {
            if(node->nodes[i])
            {
                parent->AddNode(node->nodes[i]);
            }
        }
    }
    BTreeNode<const char *, const char *, buckets> *FindNode(const char *_data)
    {
        DEBUG("Trying to find node data %s at node %x data %s\n", _data, this, this->data);
        if(!strcmp(data, _data))
            return this;
        BTreeNode<const char *, const char *, buckets> *tmp = NULL;
        int i=0, last_not_null = 0;
        for(; i<buckets; i++)
        {
            bool add = true;
            if(nodes[i] != NULL) last_not_null = i;
            if(i>0)
            {
                if(nodes[i-1] != NULL)
                {
                    if(strcmp(nodes[i-1]->data, _data) >= 0)
                    {
                        add = false;
                    }
                }
            }
            if(i<(buckets-1))
            {
                if(nodes[i+1] != NULL)
                {
                    if(strcmp(nodes[i+1]->data,_data) <= 0)
                    {
                        add = false;
                    }
                }
            }
            if(add)
            {
                if(nodes[i] != NULL)
                {
                    return nodes[i]->FindNode(_data);
                }
                break;
            }
        }
        if(i == buckets)
        {
            return nodes[last_not_null]->FindNode(_data);
        }
        return NULL;
    }
    void AddNode(BTreeNode<const char *, const char *, buckets> *node)
    {
        int i=0, last_not_null = 0;
        DEBUG("Trying to add node %x data %s to node %x data %s\n", node, node->data, this, this->data);
        for(; i<buckets; i++)
        {
            bool add = true;
            if(nodes[i] != NULL) last_not_null = i;
            if(i>0)
            {
                if(nodes[i-1] != NULL)
                {
                    if(strcmp(nodes[i-1]->data, node->data) >= 0)    //if previous is bigger or same don't add
                    {
                        add = false;
                    }
                }
            }
            if(i<(buckets-1))
            {
                if(nodes[i+1] != NULL)
                {
                    if(strcmp(nodes[i+1]->data, node->data) <=0 )   //if next is smaller or same don't add
                    {
                        add = false;
                    }
                }
            }
            if(add)
            {
                if(nodes[i] == NULL)                             //if bucket is free(NULL) assign
                {
                    nodes[i] = node;
                    node->parent = this;
                }
                else
                {
                    nodes[i]->AddNode(node);                     //if bucket is occupied add recursively
                }
                return;
            }
        }
        if(i == buckets)                                         //no suitable bucket found? take biggest which was not null, add there
        {
            nodes[last_not_null]->AddNode(node);
        }
    }
    void print(void)
    {
        DEBUG("data: %s %x start\n", data, this);
        for(int i=0; i<buckets; i++)
        {
            if(this->nodes[i]) {
                DEBUG("    nodes[%d]=\n", i);
                nodes[i]->print();
            }
        }
        DEBUG("data: %s %x end\n", data, this);
    }
};




template<int buckets>
class CharBTree
{
public:
    typedef BTreeNode<const char *, const char *, buckets>                                  CharBNode;
    typedef typename BTreeNode<const char *, const char *, buckets>::BTreeNodeAttrib        CharBNodeAttr;
    DefaultAllocator *allocator;
    Coder            *coder;
    CharBTree(DefaultAllocator *all = NULL, Coder *_coder = NULL)
    {
        if(all)
        {
            allocator = all;
        }
        else
        {
            allocator = new DefaultAllocator();
        }
        root = NULL;
        coder = _coder;
    }
    int code_fwrite(void *ptr, int size, int num, FILE *fout)
    {
        if(coder)
            coder->encode((char*)ptr, (char*)ptr, size*num);
        return fwrite(ptr, size, num, fout);
    }
    int code_fread(void *ptr, int size, int num, FILE *fin)
    {
        int res = fread(ptr, size, num, fin);
        if(coder)
            coder->decode((char*)ptr, (char*)ptr, size*num);
        return res;
    }
    void serialize(FILE *fout, BTreeNode<const char *, const char *, buckets>  *node, DefaultAllocator *allocator)
    {
        unsigned long offset = (unsigned long)node - (unsigned long)allocator->GetLowestAddress();
        code_fwrite(&offset, sizeof(unsigned long), 1, fout);
        DEBUG("Wrote node %x key %s size %d offset %x\n", (unsigned long)node, (node)->data, (node)->size, offset);
        offset = (unsigned long)node->data - (unsigned long)allocator->GetLowestAddress();
        code_fwrite(&offset, sizeof(unsigned long), 1, fout);
        offset = (unsigned long)node->value - (unsigned long)allocator->GetLowestAddress();
        code_fwrite(&offset, sizeof(unsigned long), 1, fout);
        offset = (unsigned long)node->size;
        code_fwrite(&offset, sizeof(unsigned long), 1, fout);
        code_fwrite(&node->attr, sizeof(CharBNodeAttr), 1, fout);
	int size = node->filesystem_nodes->size();
	code_fwrite(&size, sizeof(int), 1, fout);
	for(int i=0;i<size;i++)
	{
	    offset = (unsigned long)((*(node->filesystem_nodes))[i]) - (unsigned long)allocator->GetLowestAddress();
            code_fwrite(&offset, sizeof(unsigned long), 1, fout);
	}
	offset = (unsigned long)node->filesystem_parent - (unsigned long)allocator->GetLowestAddress();
        code_fwrite(&offset, sizeof(unsigned long), 1, fout);
        return;
    }
    void deserialize(FILE *fin, BTreeNode<const char *, const char *, buckets>  *node, DefaultAllocator *allocator)
    {
	node->Init();
        unsigned long offset_data = 0, offset_value = 0, offset_size = 0, offset = 0;
        code_fread(&offset_data, sizeof(unsigned long), 1, fin);
        DEBUG("Read offset_data %x\n", offset_data);
        code_fread(&offset_value, sizeof(unsigned long), 1, fin);
        DEBUG("Read offset value %x\n", offset_value);
        code_fread(&offset_size, sizeof(unsigned long), 1, fin);
        DEBUG("Read offset size %d\n", offset_size);
        code_fread(&node->attr, sizeof(CharBNodeAttr), 1, fin);
        node->data = (unsigned long)allocator->GetLowestAddress() + offset_data;
        node->value = (unsigned long)allocator->GetLowestAddress() + offset_value;
        node->size = (int)offset_size;
	int size = 0;
	code_fread(&size, sizeof(int), 1, fin);
	for(int i=0;i<size;i++)
	{
	    code_fread(&offset, sizeof(unsigned long), 1, fin);
	    BTreeNode<const char *, const char *, buckets>  *tmp = (BTreeNode<const char *, const char *, buckets>  *)(offset + (unsigned long)allocator->GetLowestAddress());
	    node->filesystem_nodes->push_back(tmp);
	}
	code_fread(&offset, sizeof(unsigned long), 1, fin);
        BTreeNode<const char *, const char *, buckets>  *filesystem_parent = (BTreeNode<const char *, const char *, buckets>  *)(offset + (unsigned long)allocator->GetLowestAddress());
	node->filesystem_parent = filesystem_parent;
        return;
    }
    void syncToDisk(const char *filename, BTreeNode<const char *, const char *, buckets> *_root = NULL)
    {
        FILE *fout = NULL;
        fout = fopen(filename, "w+b");
        if(!fout)
        {
            throw "Could not write to disk!";
        }
        DEBUG("Syncing to disk file %s\n", filename);
        vector<CharBNode*> vec;
        if(!_root)
        {
            unsigned long buckets_num = buckets;
            code_fwrite(&buckets_num, sizeof(unsigned long), 1, fout);
            DEBUG("Writing buckets num %d\n", buckets_num);
            if(root)
            {
                _root = root;
                TraverseTree(vec, _root, false);
                unsigned long size = vec.size();
                code_fwrite(&size, sizeof(unsigned long), 1, fout);
                DEBUG("Writing records num %d\n", size);
            }
            else
            {
                return;
            }
        }

        for(int n = 0; n<vec.size(); n++)
        {
            _root = vec[n];
            unsigned long offset = 0;
            serialize(fout, _root, allocator);
            for(int i=0; i<buckets; i++)
            {
                if(_root->nodes[i])
                {
                    serialize(fout, _root->nodes[i], allocator);
                }
                else
                {
                    offset = 0;
                    code_fwrite(&offset, sizeof(unsigned long), 1, fout);
                    DEBUG("Wrote node NULL offset %x\n", offset);
                }
            }
        }
        fclose(fout);
        return;
    }
    int syncFromDisk(const char *filename)
    {
        FILE *fin = NULL;
        BTreeNode<const char *, const char *, buckets> **_root = NULL, *tmp = NULL;
        fin = fopen(filename, "r+b");
        if(!fin)
        {
            DEBUG("Could not open file %s\n", filename);
            return -1;
        }
        DEBUG("Syncing from disk file %s\n", filename);
        _root = &root;
        unsigned long buckets_num = 0;
        code_fread(&buckets_num, sizeof(unsigned long), 1, fin);
        if(buckets_num != buckets)
        {
            DEBUG("Mismatch between template buckets and file! file: %d ours: %d\n", buckets_num, buckets);
            return -2;
        }
        else
        {
            DEBUG("Num buckets is %d\n", buckets_num);
        }
        unsigned long size = 0;
        code_fread(&size, sizeof(unsigned long), 1, fin);
        DEBUG("Number records is %d\n", size);
        for(int n = 0; n<size ; n++)
        {
            unsigned long offset = 0, offset_data = 0, offset_value = 0, offset_size = 0;
            code_fread(&offset, sizeof(unsigned long), 1, fin);
            DEBUG("Read initial offset %x %d\n", offset, n);
            if(offset != 0)
            {
                if(n==0)
                {
                    *_root = (unsigned long)allocator->GetLowestAddress() + offset;
                    deserialize(fin, *_root, allocator);

                    (*_root)->parent = NULL;
                    tmp = *_root;
                }
                else
                {
                    tmp = (unsigned long)allocator->GetLowestAddress() + offset;
                    deserialize(fin, tmp, allocator);
                }
                DEBUG("Extracted node %x key %s data %s offset %x\n", tmp, tmp->data, tmp->value, offset);
                for(int i=0; i<buckets; i++)
                {
                    code_fread(&offset, sizeof(unsigned long), 1, fin);
                    DEBUG("Read offset %x\n", offset);
                    if(offset != 0)
                    {
//                        (*_root)->nodes[i] = (unsigned long)allocator->GetLowestAddress() + offset;
                        DEBUG("Read offset size %d\n", offset_size);
                        tmp->nodes[i] = (unsigned long)allocator->GetLowestAddress() + offset;
                        tmp->nodes[i]->parent = tmp;
                        deserialize(fin, tmp->nodes[i], allocator);
                        DEBUG("Included node %x key %s data %s size %d offset %x\n", tmp->nodes[i], tmp->nodes[i]->data, tmp->nodes[i]->value, tmp->nodes[i]->size, offset);
                    }
                    else
                    {
                        DEBUG("Set nodes[%d]= NULL parent %x %s\n", i, tmp, tmp->data);
                        tmp->nodes[i] = NULL;
                    }
                }
            }
        }

        fclose(fin);
        return 0;
    }
    BTreeNode<const char *, const char *, buckets> * AddNode(const char *key, const char *value)
    {
        BTreeNode<const char *, const char *, buckets> *node = (CharBNode*)allocator->Allocate(sizeof( BTreeNode<const char *, const char *, buckets>));
        node->Init();
        node->data = (char*)allocator->Allocate(strlen(key)+1);
        node->value= (char*)allocator->Allocate(strlen(value)+1);
	node->size = strlen(value)+1;
        strcpy(node->data, key);
        strcpy(node->value, value);
        if(!root)
        {
            setRoot(node);
            return;
        }
        root->AddNode(node);
	return node;
    }
    BTreeNode<const char *, const char *, buckets> *AddNodeSize(const char *key, const char *value, int size)
    {
        BTreeNode<const char *, const char *, buckets> *node = (CharBNode*)allocator->Allocate(sizeof( BTreeNode<const char *, const char *, buckets>));
        node->Init();
        node->data = (char*)allocator->Allocate(strlen(key)+1);
        node->value= (char*)allocator->Allocate(size);
        node->size = size;
        strcpy(node->data, key);
        memcpy(node->value, value, size);
        if(!root)
        {
            setRoot(node);
            return;
        }
        root->AddNode(node);
	return node;
    }
    BTreeNode<const char *, const char *, buckets>  *FindNode(const char *key)
    {
        if(!root)
            return NULL;
        BTreeNode<const char *, const char *, buckets> *node = root->FindNode(key);
        return node;
    }
    void Print(void)
    {
        if(root) root->print();
    }
    void setRoot(BTreeNode<const char *, const char *, buckets> *node)
    {
        DEBUG("Node %x data %s now set as root\n", node, node->data);
        root = node;
        root->parent = NULL;
        return;
    }
    BTreeNode<const char *, const char *, buckets> * getRoot(void)
    {
        return root;
    }
    void RemoveNode(BTreeNode<const char *, const char *, buckets> *node, bool dispose = true)
    {
        if(!root) return;
        DEBUG("Removing node %x from root %x\n", node, root);
        if(node == root)
        {
            BTreeNode<const char *, const char *, buckets> *_root = root;
            root = NULL;
            bool first = true;

            for(int i=0; i<buckets; i++)
            {
                if(_root->nodes[i])
                {
                    DEBUG("%x %s\n", _root->nodes[i], _root->nodes[i]->data);
                    if(first) {
                        setRoot(_root->nodes[i]);
                        first = false;
                    }
                    else
                    {
                        root->AddNode(_root->nodes[i]);
                    }
                }
            }
            if(dispose)
            {
                DEBUG("Free data %x %s\n", _root->data, _root->data);
                allocator->Free(_root->data);
                DEBUG("Free value %x %s\n", _root->value, _root->value);
                allocator->Free(_root->value);
                DEBUG("Free node %x\n", _root);
                allocator->Free(_root);
            }
        }
        else
        {
            BTreeNode<const char *, const char *, buckets>::RemoveNode(node);
            if(dispose)
            {
                DEBUG("Free data %x %s\n", node->data, node->data);
                allocator->Free(node->data);
                DEBUG("Free value %x %s\n", node->value, node->value);
                allocator->Free(node->value);
                DEBUG("Free node %x\n", node);
                allocator->Free(node);
            }
        }
    }
    void TraverseTree(vector<CharBNode*> &vec, CharBNode *_root = NULL, bool width_or_depth = true)
    {
        if(!_root) _root = root;
        if(!_root) return;
        CharBNode *tmp = _root;
        if(width_or_depth)
        {
            queue<CharBNode*>   q;
            q.push(tmp);
            while(! q.empty() )
            {
                tmp = q.front();
                q.pop();
                vec.push_back(tmp);
                for(int i=0; i<buckets; i++)
                {
                    if(tmp->nodes[i] && tmp->nodes[i]!=tmp)
                    {
                        q.push(tmp->nodes[i]);
                    }
                }
            }
            return;
        }
        else
        {
            stack<CharBNode*>   st;
            st.push(tmp);
            while(! st.empty() )
            {
                tmp = st.top();
                st.pop();
                vec.push_back(tmp);
                for(int i=0; i<buckets; i++)
                {
                    if((tmp->nodes[i]!=NULL) && (tmp->nodes[i])!=tmp)
                    {
                        st.push(tmp->nodes[i]);
                    }
                }
            }
            return;
        }
    }
protected:
    BTreeNode<const char *, const char *, buckets> *root;
};
#endif
