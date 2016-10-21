#include "btree.h"
#include "mem_allocator.h"
#include <stdio.h>

unsigned char buffer[5000];

int main(void)
{
    MemoryAllocator2 *mem = new MemoryAllocator2(buffer, sizeof(buffer), 16, 32);
    mem->syncFromDisk("./memory_file");
    mem->PrintBlocksUsage();
    printf("Memory lowest address: %x\n", mem->GetLowestAddress());
    printf("Memory highest address: %x\n", mem->GetHighestAddress());
    CharBTree<5> tree(mem), tree2(mem);
    tree.syncFromDisk("./btree_test_file");
    tree.Print();
    BTreeNode<const char *, const char *, 5>  *node = tree.FindNode("Hello");
    if(!node)
    {
        printf("nodes not present!\n");
        tree.AddNode("Hello", "World");
        tree.AddNode("Hello2", "World2");
        tree.AddNode("Abcde", "World3");
        printf("Traverse:\n");
        vector<BTreeNode<const char *, const char *, 5>  *> vec0;
        tree.TraverseTree(vec0, NULL, false);
        for(int i=0;i<vec0.size();i++)
        {
            printf("vec0[%d]: %s %s\n", i, vec0[i]->data, vec0[i]->value);
        }
        mem->syncToDisk("./memory_file");
        tree.syncToDisk("./btree_test_file");
        delete mem;
        return 0;
    }
    else
    {
        printf("nodes loaded from file\n");
    }
    mem->PrintBlocksUsage();
    if(node)
    printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
    BTreeNode<const char *, const char *, 5>  *node2 = tree.FindNode("Hello2");
    if(node2)
    printf("Found node %x key %s value %s\n", (unsigned long)node2, node2->data, node2->value);

    printf("tree:\n");
    tree.Print();
    printf("*****************************************\n");
    printf("tree2 before sync:\n");
    tree2.Print();
    tree2.syncFromDisk("./btree_test_file");
    printf("tree2 after sync:\n");
    tree2.Print();
    node = tree2.FindNode("Hello");
    if(node)
    printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
    node2 = tree2.FindNode("Hello2");
    if(node2)
    printf("Found node %x key %s value %s\n", (unsigned long)node2, node2->data, node2->value);
//    tree2.AddNode("Hello50", "World3");
    vector<BTreeNode<const char *, const char *, 5>  *> vec, vec2;
    tree2.TraverseTree(vec, NULL, false);
    for(int i=0;i<vec.size();i++)
    {
        printf("vec[%d]: %s %s\n", i, vec[i]->data, vec[i]->value);
    }
    tree2.TraverseTree(vec2, NULL, true);
    for(int i=0;i<vec2.size();i++)
    {
        printf("vec2[%d]: %s %s\n", i, vec2[i]->data, vec2[i]->value);
    }
    printf("*****************************************\n");
    mem->syncToDisk("./memory_file");
    delete mem;
    return 0;
};