#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
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
        for(;i<buckets;i++)
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
            for(int i=0;i<buckets;i++)
            {
                if(parent->nodes[i] == node)
                {
                    parent->nodes[i] = NULL;
                    break;
                }
            }
            for(int i=0;i<buckets;i++)
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
        for(i=0;i<buckets;i++)
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
        if(nodes[last_not_null])
                return nodes[last_not_null]->FindNode(_data);
        return NULL;
    }
    void AddNode(BTreeNode<const char *, const char *, buckets> *node)
    {
        int i=0;
        for(;i<buckets;i++)
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
            nodes[i-1]->AddNode(node);
    }
    void print(void)
    {
        printf("data: %s %x start\n", data, this);
        for(int i=0;i<buckets;i++)
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
            for(int i=0;i<buckets;i++)
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
    void TraverseTree(vector<CharBNode*> &vec, CharBNode *_root = NULL)
    {
        if(!_root) _root = root;
        if(!_root) return;
        vec.push_back(_root);
        for(int i=0;i<buckets;i++)
        {
            if(_root->nodes[i])
            {
                TraverseTree(vec, _root->nodes[i]);
            }
        }
        return;
    }
    protected:
    BTreeNode<const char *, const char *, buckets> *root;
};
#if 0
int main(void)
{
    CharBTree<5>    tree = CharBTree<5>();
    tree.AddNode("1", "one");
    tree.AddNode("2", "two");
    CharBTree<5>::CharBNode *node = tree.FindNode("1");
    printf("%s %s", node->data, node->value);
    node = tree.FindNode("2");
    printf("%s %s", node->data, node->value);
    tree.RemoveNode(node, false);
    node = tree.FindNode("2");
    if(!node) printf("Not found\n");
    else printf("%s %s", node->data, node->value);
    tree.Print();
    return 0;
}
#endif