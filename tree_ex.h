#ifndef TREE_EX_H
#define TREE_EX_H

#include <stdio.h>
#include <stack>
#include <string>
#include <vector>

using namespace std;


template<class T, class D>
struct BinaryTreeNode
{
    BinaryTreeNode()
    {
        right = NULL;
        left  = NULL;
        parent = NULL;
    }
    void Print(void)
    {
    }
    BinaryTreeNode<T,D> *right;
    BinaryTreeNode<T,D> *left;
    BinaryTreeNode<T,D> *parent;
    T              data;
    D              value;
};

template<class D>
struct BinaryTreeNode<int, D>
{
    BinaryTreeNode()
    {
        right = NULL;
        left  = NULL;
        parent= NULL;
    }
    void Print(void)
    {
        printf("%d\n", data);
    }
    BinaryTreeNode *right;
    BinaryTreeNode *left;
    BinaryTreeNode<int,D> *parent;
    int             data;
    D               value;
};

template<class T, class D>
class BinaryTree
{
public:
    BinaryTree()
    {
        root = NULL;
    }
    void Add(T data, D value)
    {
        BinaryTreeNode<T,D> *node = new BinaryTreeNode<T,D>();
        node->data = data;
        node->value = value;
        AddNode(node);
    }
    void AddNode(BinaryTreeNode<T,D>	*node, BinaryTreeNode<T,D>  *_root = NULL, bool non_recurs = true)
    {

        if(!root)
        {
            root = node;
            root->parent = NULL;
            return;
        }

        if(!_root)
            _root = root;

        if(non_recurs)
            return AddNodeNonRecurs(node, _root);

        if( _root->data < node-> data)
        {
            if(_root->right)
            {
                AddNode(node, _root->right);
            }
            else
            {
                _root->right = node;
                node->parent = _root;
            }
        }

        if( _root->data >= node-> data)
        {
            if(_root->left)
            {
                AddNode(node, _root->left);
            }
            else
            {
                _root->left = node;
                node->parent = _root;
            }
        }
        return;
    }

    void AddNodeNonRecurs(BinaryTreeNode<T,D> *node, BinaryTreeNode<T,D> *_root)
    {
        stack< BinaryTreeNode<T,D>* > st;
        st.push(_root);
        while(!st.empty())
        {
            BinaryTreeNode<T,D>  *current_root = st.top();
            st.pop();

            if( current_root->data < node-> data)
            {
                if(current_root->right)
                {
                    st.push(current_root->right);
                }
                else
                {
                    current_root->right = node;
                    node->parent = current_root;
                }
            }

            if( current_root->data >= node-> data)
            {
                if(current_root->left)
                {
                    st.push(current_root->left);
                }
                else
                {
                    current_root->left = node;
                    node->parent = current_root;
                }
            }

        }
        return;


    }
    void RemoveNode(BinaryTreeNode<T,D> *node)
    {
        if(node->parent)
        {
            if(node->parent->left == node) //we are left
            {
                if(node->right)
                {
                    node->parent->left = node->right;
                    node->right->parent = node->parent;
                    if(node->left)
                        AddNode(node->left, node->right);
                }
                else
                {
                    if(node->left)
                    {
                        node->parent->left = node->left;
                        node->left->parent = node->parent;
                        AddNode(node->left, node->parent);
                    }
                }
            }
            if(node->parent->right == node) //we are left
            {
                if(node->left)
                {
                    node->parent->right = node->left;
                    node->left->parent = node->parent;
                    if(node->right)
                        AddNode(node->right, node->left);
                }
                else
                {
                    if(node->right)
                    {
                        node->parent->right = node->right;
                        node->right->parent = node->parent;
                        AddNode(node->right, node->parent);
                    }
                }
            }
        }
        else
        {
            AddNode(root->left, root->right);
            root = root->right;
        }
    }
    void TraverseTree(BinaryTreeNode<T,D> *_root, vector<BinaryTreeNode<T,D> *> &vec )
    {
        if(!_root)
            _root = root;

        vec.push_back(_root);

        if(_root->left)
            TraverseTree(_root->left, vec);

        if(_root->right)
            TraverseTree(_root->right, vec);


    }
    BinaryTreeNode<T,D> * Search(T value)
    {
        BinaryTreeNode<T,D> *tmp = root;
        while(tmp != NULL)
        {
#if DEBUG
            printf("%d %d\n", tmp->data, value);
#endif
            if(tmp->data == value)
            {
                break;
            }
            if(tmp->data <= value)
            {
                tmp = tmp->right;
            }
            else
            {
                tmp = tmp->left;
            }
        }
        return tmp;
    }
protected:
    BinaryTreeNode<T,D> *root;
};


template<>
class BinaryTree<const char *, const char *>
{
public:
    BinaryTree()
    {
        root = NULL;
    }
    void Add(const char * data, const char * value)
    {
        BinaryTreeNode<const char *,const char *> *node = new BinaryTreeNode<const char *,const char *>();
        node->data = data;
        node->value = value;
        AddNode(node);
    }
    void AddNode(BinaryTreeNode<const char *,const char *>  *node, BinaryTreeNode<const char *,const char *>  *_root = NULL, bool non_recurs = true)
    {

        if(!root)
        {
            root = node;
            root->parent = NULL;
            return;
        }

        if(!_root)
            _root = root;

        if(non_recurs)
        {
            return AddNodeNonRecurs(node, _root);
        }

        if( _root->data < node-> data)
        {
            if(_root->right)
            {
                AddNode(node, _root->right);
            }
            else
            {
                _root->right = node;
                node->parent = _root;
            }
        }

        if( _root->data >= node-> data)
        {
            if(_root->left)
            {
                AddNode(node, _root->left);
            }
            else
            {
                _root->left = node;
                node->parent = _root;
            }
        }
        return;
    }
    void RemoveNode(BinaryTreeNode<const char *,const char*> *node)
    {
#if DEBUG
        printf("root:%s node:%s", root->data, node->data);
#endif
        if(node->parent)
        {
            if(node->parent->left == node) //we are left
            {
                if(node->right)
                {
                    node->parent->left = node->right;
                    node->right->parent = node->parent;
                    if(node->left)
                        AddNode(node->left, node->right);
                }
                else
                {
                    if(node->left)
                    {
                        node->parent->left = node->left;
                        node->left->parent = node->parent;
                        AddNode(node->left, node->parent);
                    }
                    else
                    {
                        node->parent->left = NULL;
                    }
                }
            }
            if(node->parent->right == node) //we are left
            {
                if(node->left)
                {
                    node->parent->right = node->left;
                    node->left->parent = node->parent;
                    if(node->right)
                        AddNode(node->right, node->left);
                }
                else
                {
                    if(node->right)
                    {
                        node->parent->right = node->right;
                        node->right->parent = node->parent;
                        AddNode(node->right, node->parent);
                    }
                    else
                    {
                        node->parent->right = NULL;
                    }
                }
            }
        }
        else
        {
            if(root->right)
            {
                AddNode(root->left, root->right);
                root = root->right;
                root->parent = NULL;
            }
            else
            {
                root = root->left;
                root->parent = NULL;
            }
        }
#if DEBUG
        printf("root:%s node:%s", root->data, node->data);
#endif
    }
    void AddNodeNonRecurs(BinaryTreeNode<const char *,const char *> *node, BinaryTreeNode<const char *,const char *> *_root)
    {
        stack< BinaryTreeNode<const char *,const char *>* > st;
        st.push(_root);
        while(!st.empty())
        {
            BinaryTreeNode<const char *,const char *>  *current_root = st.top();
            st.pop();

            if( current_root->data < node-> data)
            {
                if(current_root->right)
                {
                    st.push(current_root->right);
                }
                else
                {
                    current_root->right = node;
                    node->parent = current_root;
                }
            }

            if( current_root->data >= node-> data)
            {
                if(current_root->left)
                {
                    st.push(current_root->left);
                }
                else
                {
                    current_root->left = node;
                    node->parent = current_root;
                }
            }

        }
        return;


    }
    void TraverseTree(BinaryTreeNode<const char *,const char *> *_root, vector<BinaryTreeNode<const char *,const char *> *> &vec )
    {
        if(!_root)
            _root = root;

        if(!_root)
            return;

        vec.push_back(_root);

        if(_root->left)
            TraverseTree(_root->left, vec);

        if(_root->right)
            TraverseTree(_root->right, vec);

    }
    void PrintTree(BinaryTreeNode<const char*, const char*> *_root = NULL)
    {
        if(!_root)
            _root = root;

#if DEBUG
        printf("\n%s \n", _root->data);
        printf("\nLeft:\n");
#endif
        if(_root->left)
        {
            PrintTree(_root->left);
        }
#if DEBUG
        printf("\nRight:\n");
#endif
        if(_root->right)
        {
            PrintTree(_root->right);
        }
    }
    BinaryTreeNode<const char *,const char *> * Search(const char * value)
    {
        BinaryTreeNode<const char *,const char *> *tmp = root;
        while(tmp != NULL)
        {
            if(!strcmp(tmp->data, value))
            {
                break;
            }
            if(strcmp(tmp->data,value)<0)
            {
                tmp = tmp->right;
            }
            else
            {
                tmp = tmp->left;
            }
        }
        return tmp;
    }
protected:
    BinaryTreeNode<const char *,const char *> *root;
};
#endif