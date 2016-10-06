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
#if 0
bool operator < (char *str1, char *str2)
{
    return string(str1)<string(str2);
}
#endif

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
//	printf("\nLeft:\n");
	if(_root->left)
	    TraverseTree(_root->left, vec);
//	printf("\nRight:\n");
	if(_root->right)
	    TraverseTree(_root->right, vec);
//	printf("\n");
	
    }
    BinaryTreeNode<T,D> * Search(T value)
    {
       BinaryTreeNode<T,D> *tmp = root;
       while(tmp != NULL)
       {
          printf("%d %d\n", tmp->data, value);
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
    void AddNode(BinaryTreeNode<const char *,const char *>	*node, BinaryTreeNode<const char *,const char *>  *_root = NULL, bool non_recurs = true)
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
    void RemoveNode(BinaryTreeNode<const char *,const char*> *node)
    {
        printf("root:%s node:%s", root->data, node->data);
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
        printf("root:%s node:%s", root->data, node->data);
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
//	printf("\nLeft:\n");
	if(_root->left)
	    TraverseTree(_root->left, vec);
//	printf("\nRight:\n");
	if(_root->right)
	    TraverseTree(_root->right, vec);
//	printf("\n");
	
    }
    void PrintTree(BinaryTreeNode<const char*, const char*> *_root = NULL)
    {
        if(!_root)
            _root = root;
        printf("\n%s \n", _root->data);
        printf("\nLeft:\n");
        if(_root->left)
            PrintTree(_root->left);
        printf("\nRight:\n");
        if(_root->right)
            PrintTree(_root->right);
        printf("\n");
    }
    BinaryTreeNode<const char *,const char *> * Search(const char * value)
    {
       BinaryTreeNode<const char *,const char *> *tmp = root;
       while(tmp != NULL)
       {
//          printf("%d %d\n", tmp->data, value);
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

#if 0


int main(void)
{
    BinaryTree<int> t;
    t.Add(2);
    t.Add(1);
    t.Add(3);
    t.Add(30);
    t.Add(5);
    t.PrintTree();
    BinaryTreeNode<int> *node = t.Search(30);
    t.RemoveNode(node);
    if(!node)
      printf("Not found\n");
    else
      printf("%d ", node->data);
    return 0;
}
#endif