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
    BTreeNode<const char *, const char *, 5>  *node = tree.FindNode("Hello"), *tmp = NULL, *node2 = NULL;
    if(!node)
    {
        printf("nodes not present! populating tree with nodes...\n");
        tree.AddNode("Hello", "World");
        tree.AddNode("Hello2", "World2");
        tree.AddNode("Hello3", "World3");
        tree.AddNode("Abcde", "World3");
        tree.AddNode("Hello8", "World8");
        printf("Traverse:\n");
        vector<BTreeNode<const char *, const char *, 5>  *> vec0;
        tree.TraverseTree(vec0, NULL, false);
        for(int i=0; i<vec0.size(); i++)
        {
            printf("vec0[%d]: %s %s\n", i, vec0[i]->data, vec0[i]->value);
        }
        mem->syncToDisk("./memory_file");
        tree.syncToDisk("./btree_test_file");

        node = tree.FindNode("Hello");
        if(node)
        {
            printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
            printf("Now removing it\n");
            tree.RemoveNode(node);
        }
        else
        {
            printf("Required node Hello not found\n");
        }
        node = tree.FindNode("Hello2");
        if(node)
        {
            printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
            printf("Now removing it\n");
            tree.RemoveNode(node);
        }
        else
        {
            printf("Required node Hello2 not found\n");
        }
        node = tree.FindNode("Hello3");
        if(node)
        {
            printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
            printf("Now removing it\n");
            tree.RemoveNode(node);
        }
        else
        {
            printf("Required node Hello3 not found\n");
        }
        node = tree.FindNode("Hello8");
        if(node)
        {
            printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
            printf("Now removing it\n");
            tree.RemoveNode(node);
        }
        else
        {
            printf("Required node Hello8 not found\n");
        }
        node = tree.FindNode("Abcde");
        if(node)
        {
            printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
            printf("Now removing it\n");
            tree.RemoveNode(node);
        }
        else
        {
            printf("Required node Abcde not found\n");
        }
        mem->PrintBlocksUsage();
        delete mem;
        return 0;
    }
    else
    {
        printf("nodes loaded from file\n");
    }
    mem->PrintBlocksUsage();
    printf("tree:\n");
    tree.Print();
    printf("*****************************************\n");
    printf("tree2 before sync:\n");
    tree2.Print();
    tree2.syncFromDisk("./btree_test_file");
    printf("tree2 after sync:\n");
    tree2.Print();
    vector<BTreeNode<const char *, const char *, 5>  *> vec, vec2;
    tree2.TraverseTree(vec, NULL, false);
    for(int i=0; i<vec.size(); i++)
    {
        printf("vec[%d] %x: %s %s\n", i, vec[i], vec[i]->data, vec[i]->value);
    }
    tree2.TraverseTree(vec2, NULL, true);
    for(int i=0; i<vec2.size(); i++)
    {
        printf("vec2[%d] %x: %s %s\n", i, vec2[i], vec2[i]->data, vec2[i]->value);
    }
    printf("*****************************************\n");
    node = tree2.FindNode("Hello");
    if(node)
        printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
    node2 = tree2.FindNode("Hello2");
    if(node2)
        printf("Found node %x key %s value %s\n", (unsigned long)node2, node2->data, node2->value);
    node = tree2.FindNode("Hello3");
    if(node)
        printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
    node = tree2.FindNode("Hello8");
    if(node)
        printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
    node = tree2.FindNode("Abcde");
    if(node)
    {
        printf("Found node %x key %s value %s\n", (unsigned long)node, node->data, node->value);
        printf("Now removing it\n");
        tree2.RemoveNode(node);
    }
    tree2.Print();
    node = tree2.FindNode("Abcde");
    if(!node)
    {
        printf("Not Found node Abcde\n");
    }
    else
    {
        printf("Node found even though is shouldn't!!!\n");
        return -1;
    }
    vector<BTreeNode<const char *, const char *, 5>  *> vec3, vec4;
    tree2.TraverseTree(vec3, NULL, false);
    for(int i=0; i<vec3.size(); i++)
    {
        printf("vec3[%d]: %s %s\n", i, vec3[i]->data, vec3[i]->value);
    }
    tree2.TraverseTree(vec4, NULL, true);
    for(int i=0; i<vec4.size(); i++)
    {
        printf("vec4[%d]: %s %s\n", i, vec4[i]->data, vec4[i]->value);
    }
    printf("*****************************************\n");
//    mem->syncToDisk("./memory_file");
    delete mem;
    return 0;
};