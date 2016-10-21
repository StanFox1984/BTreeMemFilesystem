#ifndef BTREE_H
#define BTREE_H
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <queue>
#include <stack>

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
    BTreeNode<const char *, const char *, buckets>    *nodes[buckets];
    const char *data;
    const char *value;
    BTreeNode<const char *, const char *, buckets>    *parent;
    void Init(void)
    {
        memset(nodes, NULL, sizeof(void*)*buckets);
        parent = NULL;
        data = NULL;
        value = NULL;
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
                break;
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
        if(!strcmp(data, _data))
            return this;
        int i=0;
        int last_not_null = 0;
        for(i=0; i<buckets; i++)
        {
            if(nodes[i]!=NULL)
            {
                last_not_null = i;
                if(i==0)
                {
                    if(strcmp(_data,nodes[i]->data)<=0)
                        return nodes[i]->FindNode(_data);
                }
                else
                {
                    if(nodes[i-1])
                    {
                        if(strcmp(_data,nodes[i-1]->data)>0 && strcmp(_data,nodes[i]->data)<=0)
                        {
                            return nodes[i]->FindNode(_data);
                        }
                    }
                }
            }
        }
        if(i==buckets)
        {
            if(nodes[last_not_null])
            {
                return nodes[last_not_null]->FindNode(_data);
            }
        }
        return NULL;
    }
    void AddNode(BTreeNode<const char *, const char *, buckets> *node)
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
                        node->parent = this;
                        break;
                    }
                }
                else
                {
                    nodes[i] = node;
                    node->parent = this;
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
        if(i == buckets)
        {
            nodes[i-1]->AddNode(node);
        }
    }
    void print(void)
    {
        printf("data: %s %x start\n", data, this);
        for(int i=0; i<buckets; i++)
        {
            if(this->nodes[i]) {
                printf("nodes[%d]:\n", i);
                nodes[i]->print();
            }
        }
        printf("data: %s %x end\n", data, this);
    }
};

template<int buckets>
class CharBTree
{
public:
    typedef BTreeNode<const char *, const char *, buckets>   CharBNode;
    DefaultAllocator *allocator;
    CharBTree(DefaultAllocator *all = NULL)
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
    }
    void syncToDisk(const char *filename, BTreeNode<const char *, const char *, buckets> *_root = NULL)
    {
        FILE *fout = NULL;
        fout = fopen(filename, "w+b");
        if(!fout)
        {
            throw "Could not write to disk!";
        }
        printf("Syncing to disk file %s\n", filename);
        vector<CharBNode*> vec;
        if(!_root)
        {
            unsigned long buckets_num = buckets;
            fwrite(&buckets_num, sizeof(unsigned long), 1, fout);
            printf("Writing buckets num %d\n", buckets_num);
            if(root)
            {
                _root = root;
                TraverseTree(vec, _root, false);
                unsigned long size = vec.size();
                fwrite(&size, sizeof(unsigned long), 1, fout);
                printf("Writing records num %d\n", size);
            }
            else
            {
                return;
            }
        }

        for(int n = 0; n<vec.size(); n++)
        {
            _root = vec[n];
            unsigned long offset = (unsigned long)_root - (unsigned long)allocator->GetLowestAddress();
            fwrite(&offset, sizeof(unsigned long), 1, fout);
            printf("Wrote node %x key %s data %s offset %x\n", (unsigned long)_root, (_root)->data, (_root)->value, offset);
            offset = (unsigned long)_root->data - (unsigned long)allocator->GetLowestAddress();
            fwrite(&offset, sizeof(unsigned long), 1, fout);
            offset = (unsigned long)_root->value - (unsigned long)allocator->GetLowestAddress();
            fwrite(&offset, sizeof(unsigned long), 1, fout);
            for(int i=0; i<buckets; i++)
            {
                if(_root->nodes[i])
                {
                    offset = (unsigned long)_root->nodes[i] - (unsigned long)allocator->GetLowestAddress();
                    fwrite(&offset, sizeof(unsigned long), 1, fout);
                    printf("Wrote node %x key %s data %s offset %x\n", (unsigned long)_root->nodes[i], (_root->nodes[i])->data, (_root->nodes[i])->value, offset);
                    offset = (unsigned long)_root->nodes[i]->data - (unsigned long)allocator->GetLowestAddress();
                    fwrite(&offset, sizeof(unsigned long), 1, fout);
                    offset = (unsigned long)_root->nodes[i]->value - (unsigned long)allocator->GetLowestAddress();
                    fwrite(&offset, sizeof(unsigned long), 1, fout);
                }
                else
                {
                    offset = 0;
                    fwrite(&offset, sizeof(unsigned long), 1, fout);
                    printf("Wrote node NULL offset %x\n", offset);
                }
            }
        }
        fclose(fout);
        return;
    }
    void syncFromDisk(const char *filename)
    {
        FILE *fin = NULL;
        BTreeNode<const char *, const char *, buckets> **_root = NULL, *tmp = NULL;
        fin = fopen(filename, "r+b");
        if(!fin)
        {
            printf("Could not open file %s\n", filename);
            return;
        }
        printf("Syncing from disk file %s\n", filename);
        _root = &root;
        unsigned long buckets_num = 0;
        fread(&buckets_num, sizeof(unsigned long), 1, fin);
        if(buckets_num != buckets)
        {
            printf("Mismatch between template buckets and file! file: %d ours: %d\n", buckets_num, buckets);
            throw "Mismatch between template buckets and file!";
        }
        else
        {
            printf("Num buckets is %d\n", buckets_num);
        }
        unsigned long size = 0;
        fread(&size, sizeof(unsigned long), 1, fin);
        printf("Number records is %d\n", size);
        for(int n = 0; n<size ; n++)
        {
            unsigned long offset = 0, offset_data = 0, offset_value = 0;
            fread(&offset, sizeof(unsigned long), 1, fin);
            printf("Read initial offset %x %d\n", offset, n);
            fread(&offset_data, sizeof(unsigned long), 1, fin);
            printf("Read offset_data %x\n", offset_data);
            fread(&offset_value, sizeof(unsigned long), 1, fin);
            printf("Read offset value %x\n", offset_value);
            if(offset != 0)
            {
                if(n==0)
                {
                    *_root = (unsigned long)allocator->GetLowestAddress() + offset;
                    (*_root)->data = (unsigned long)allocator->GetLowestAddress() + offset_data;
                    (*_root)->value = (unsigned long)allocator->GetLowestAddress() + offset_value;
                    tmp = *_root;
                }
                else
                {
                    tmp = (unsigned long)allocator->GetLowestAddress() + offset;
                    tmp->data = (unsigned long)allocator->GetLowestAddress() + offset_data;
                    tmp->value = (unsigned long)allocator->GetLowestAddress() + offset_value;
                }
                printf("Extracted node %x key %s data %s offset %x\n", tmp, tmp->data, tmp->value, offset);
                for(int i=0; i<buckets; i++)
                {
                    fread(&offset, sizeof(unsigned long), 1, fin);
                    printf("Read offset %x\n", offset);
                    if(offset != 0)
                    {
//                        (*_root)->nodes[i] = (unsigned long)allocator->GetLowestAddress() + offset;
                        fread(&offset_data, sizeof(unsigned long), 1, fin);
                        printf("Read offset_data %x\n", offset_data);
                        fread(&offset_value, sizeof(unsigned long), 1, fin);
                        printf("Read offset value %x\n", offset_value);
                        tmp->nodes[i] = (unsigned long)allocator->GetLowestAddress() + offset;
                        tmp->nodes[i]->data = (unsigned long)allocator->GetLowestAddress() + offset_data;
                        tmp->nodes[i]->value = (unsigned long)allocator->GetLowestAddress() + offset_value;
                        printf("Included node %x key %s data %s offset %x\n", tmp->nodes[i], tmp->nodes[i]->data, tmp->nodes[i]->value, offset);
                    }
                }
            }
        }

        fclose(fin);
        return;
    }
    void AddNode(const char *key, const char *value)
    {
        BTreeNode<const char *, const char *, buckets> *node = (CharBNode*)allocator->Allocate(sizeof( BTreeNode<const char *, const char *, buckets>));
        node->Init();
        node->data = (char*)allocator->Allocate(strlen(key)+1);
        node->value= (char*)allocator->Allocate(strlen(value)+1);
        strcpy(node->data, key);
        strcpy(node->value, value);
        if(!root)
        {
            root = node;
            root->parent = NULL;
            return;
        }
        root->AddNode(node);
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
    void RemoveNode(BTreeNode<const char *, const char *, buckets> *node, bool dispose = true)
    {
        if(!root) return;
        if(node == root)
        {
            BTreeNode<const char *, const char *, buckets> *_root = root;
            root = NULL;
            for(int i=0; i<buckets; i++)
            {
                if(_root->nodes[i])
                {
                    AddNode(_root->nodes[i]->data, _root->nodes[i]->value);
                }
            }
            if(dispose)
            {
                allocator->Free(_root->data);
                allocator->Free(_root->value);
                allocator->Free(_root);
            }
        }
        else
        {
            BTreeNode<const char *, const char *, buckets>::RemoveNode(node);
            if(dispose)
            {
                allocator->Free(node->data);
                allocator->Free(node->value);
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
                    if(tmp->nodes[i] && tmp->nodes[i]!=tmp)
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
