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
            if(nodes[i]) {
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
        vector<CharBNode*> vec;
        if(!_root)
        {
            if(root)
            {
                _root = root;
                unsigned long buckets_num = buckets;
                fwrite(&buckets_num, sizeof(unsigned long), 1, fout);
                TraverseTree(vec, _root, false);
                unsigned long size = vec.size();
                fwrite(&size, sizeof(unsigned long), 1, fout);
            }
            else
            {
                return;
            }
        }

        for(int n = 0;n<vec.size();n++)
        {
            _root = vec[n];
            unsigned long offset = (unsigned long)_root - (unsigned long)allocator->GetLowestAddress();
            fwrite(&offset, sizeof(unsigned long), 1, fout);
            for(int i=0; i<buckets; i++)
            {
                if(_root->nodes[i])
                {
                    offset = (unsigned long)_root->nodes[i] - (unsigned long)allocator->GetLowestAddress();
                    fwrite(&offset, sizeof(unsigned long), 1, fout);
                }
                else
                {
                    offset = 0;
                    fwrite(&offset, sizeof(unsigned long), 1, fout);
                }
            }
        }
        fclose(fout);
        return;
    }
    void syncFromDisk(const char *filename)
    {
        FILE *fin = NULL;
        BTreeNode<const char *, const char *, buckets> **_root = NULL;
        fin = fopen(filename, "r+b");
        if(!fin)
        {
            return;
        }
        _root = &root;
        unsigned long buckets_num = 0;
        fread(&buckets_num, sizeof(unsigned long), 1, fin);
        if(buckets_num != buckets)
        {
            throw "Mismatch between template buckets and file!";
        }
        unsigned long size = 0;
        fread(&size, sizeof(unsigned long), 1, fin);
        queue<CharBNode **>  q;
        q.push(_root);
        for(int n = 0; n<size ;n++)
        {
            while(! q.empty() )
            {
                _root = q.front();
                q.pop();
                unsigned long offset = 0;
                fread(&offset, sizeof(unsigned long), 1, fin);
                *_root = (unsigned long)allocator->GetLowestAddress() + offset;
                for(int i=0; i<buckets; i++)
                {
                    fread(&offset, sizeof(unsigned long), 1, fin);
                    if(offset != 0)
                    {
//                        (*_root)->nodes[i] = (unsigned long)allocator->GetLowestAddress() + offset;
                        q.push(&((*_root)->nodes[i]));
                    }
                    else
                    {
//                        (*_root)->nodes[i] = NULL;
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
        node->data = key;
        node->value = value;
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
                allocator->Free(_root);
        }
        else
        {
            BTreeNode<const char *, const char *, buckets>::RemoveNode(node);
            if(dispose)
                allocator->Free(node);
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
                    if(tmp->nodes[i])
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
                    if(tmp->nodes[i])
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
