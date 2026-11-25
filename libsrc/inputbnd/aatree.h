#pragma once

#include <types.h>
#include <assert.h>



template <typename T>
class aatree
{
public:
	struct aa_node
	{
		char* name;
		T* info;
		int info_array_size;
		aa_node* left;
		aa_node* right;
		aa_node* parent;
		short level;
		int visited;
	};

	aatree();
	~aatree();

	void Add(char* pName, T* pInfo, long iInfoArraySize);
	int Delete(char* pName, int iDeleteInfo, T* pInfo);
	void DeleteAll(int pDeleteInfo);
	T* Find(char* pName);
	void ChangeInfo(char* pName, T* pInfo, long iInfoArraySize);
	int GetNumNodes();
	void ResetVisited(aa_node* pCurrent);
	void VisitBefore(T* pInfo, aa_node* pCurrent);
	T* GetNextInOrder(char* pStr);

private:
	void Insert(char* pName, T* pInfo, aa_node** ppOutNode, aa_node* pParent, long iInfoArraySize);
	void ChangeNodeInfo(char* pName, T* pInfo, aa_node* pCurrent, long iInfoArraySize);
	void Skew(aa_node** ppOutNode);
	void Split(aa_node** ppOutNode);
	aa_node* RotateWithLeftChild(aa_node* pNode);
	aa_node* RotateWithRightChild(aa_node* pNode);
	void ResetParents(aa_node* cur, aa_node* parent);
	int Remove(char* pName, aa_node** ppOutNode, int iDeleteInfo, T* pInfo);
	void FreeTree(aa_node** ppOutNode, int iDeleteInfo);
	T* Search(char* pName, aa_node* pCurrent);

	int num_nodes;
	aa_node* root;
	aa_node* null_node;
	aa_node* cur_visited;
};

template <typename T>
inline aatree<T>::aatree()
{
	#line 70
	null_node = root = new aa_node();
	num_nodes = 0;
	null_node->level = 0;
	null_node->name = NULL;
	null_node->info = NULL;
	root = null_node;
	null_node->right = root;
	null_node->left = null_node->right;
}

template <typename T>
aatree<T>::~aatree()
{
	DeleteAll(1);
}

template <typename T>
void aatree<T>::Add(char* name, T* info, long info_array_size)
{
	Insert(name, info, &root, null_node, info_array_size);
}

template <typename T>
int aatree<T>::Delete(char* name, int delete_info, T* info)
{
	int ret_val = Remove(name, &root, delete_info, info);
	ResetParents(root, NULL);
	return ret_val;
}
template <typename T>
void aatree<T>::DeleteAll(int delete_info)
{
	FreeTree(&root, delete_info);
	root = null_node;
}
template <typename T>
void aatree<T>::ChangeInfo(char* pName, T* pInfo, long iInfoArraySize)
{
	ChangeNodeInfo(pName, pInfo, root, iInfoArraySize);
}
template <typename T>
void aatree<T>::ChangeNodeInfo(char* pName, T* pInfo, typename aatree<T>::aa_node* pCurrent, long iInfoArraySize)
{
	if (pCurrent == null_node)
		return;
	if (strcmp(pName, pCurrent->name) == 0)
	{
		pCurrent->info = pInfo;
		pCurrent->info_array_size = iInfoArraySize;
	}
	else
	{
		if (strcmp(pName, pCurrent->name) < 0)
		{
			ChangeNodeInfo(pName, pInfo, pCurrent->left, iInfoArraySize);
		}
		else
		{
			ChangeNodeInfo(pName, pInfo, pCurrent->right, iInfoArraySize);
		}
	}
}
template <typename T>
int aatree<T>::GetNumNodes()
{
	return num_nodes;
}
template<typename T>
void aatree<T>::Insert(char* name, T* info, typename aatree<T>::aa_node** ppOutNode, typename aatree<T>::aa_node* parent, long info_array_size)
{
	assert(ppOutNode != NULL);
	if (*ppOutNode == null_node)
	{
		++num_nodes;
		*ppOutNode = new aa_node();

#line 146
		(*ppOutNode)->name = new char[strlen(name) + 1];
		strcpy((*ppOutNode)->name, name);

		(*ppOutNode)->info = info;
		(*ppOutNode)->info_array_size = info_array_size;
		(*ppOutNode)->right = null_node;
		(*ppOutNode)->left = (*ppOutNode)->right;
		(*ppOutNode)->parent = parent;
		(*ppOutNode)->level = 1;
		(*ppOutNode)->visited = 0;

		return;
	}

	if (strcmp(name, (*ppOutNode)->name) <= 0)
		Insert(name, info, &(*ppOutNode)->left, *ppOutNode, info_array_size);
	else if (strcmp(name, (*ppOutNode)->name) > 0)
		Insert(name, info, &(*ppOutNode)->right, *ppOutNode, info_array_size);
	else
		return;
	Skew(ppOutNode);
	Split(ppOutNode);
}

template<typename T>
void aatree<T>::Skew(typename aatree<T>::aa_node** ppOutNode)
{
	if ((*ppOutNode)->left->level == (*ppOutNode)->level)
		*ppOutNode = RotateWithLeftChild(*ppOutNode);
}

template<typename T>
void aatree<T>::Split(typename aatree<T>::aa_node** ppOutNode)
{
	if ((*ppOutNode)->right->right->level == (*ppOutNode)->level)
	{
		*ppOutNode = RotateWithRightChild(*ppOutNode);
		++(*ppOutNode)->level;
	}
}

template<typename T>
typename aatree<T>::aa_node* aatree<T>::RotateWithLeftChild(typename aatree<T>::aa_node* pNode)
{
	aa_node* tmp = pNode->left;
	pNode->left = tmp->right;
	tmp->right->parent = pNode;
	tmp->right = pNode;
	tmp->parent = pNode->parent;
	pNode->parent = tmp;

	return tmp;
}

template<typename T>
typename aatree<T>::aa_node* aatree<T>::RotateWithRightChild(typename aatree<T>::aa_node* pNode)
{
	aa_node* tmp = pNode->right;
	pNode->right = tmp->left;
	tmp->left->parent = pNode;
	tmp->left = pNode;
	tmp->parent = pNode->parent;
	pNode->parent = tmp;

	return tmp;
}

template<typename T>
void aatree<T>::ResetParents(typename aatree<T>::aa_node* cur, typename aatree<T>::aa_node* parent)
{
	if (!parent)
		parent = null_node;

	if (cur == null_node)
		return;

	cur->parent = parent;
	ResetParents(cur->left, cur);
	ResetParents(cur->right, cur);
}

template <typename T>
T* aatree<T>::Find(char* name)
{
	return Search(name, root);
}

template<typename T>
T* aatree<T>::Search(char* name, typename aatree<T>::aa_node* cur)
{
	if (cur == null_node)
		return NULL;

	int cmp = strcmp(name, cur->name);
	if (cmp > 0)
		return Search(name, cur->right);
	if (cmp < 0)
		return Search(name, cur->left);

	cur_visited = cur;
	return cur->info;
}

template <typename T>
void aatree<T>::ResetVisited(typename aatree<T>::aa_node* cur)
{
	if (!cur)
	{
		cur = root;
		cur_visited = cur;
	}

	if (cur != null_node)
	{
		ResetVisited(cur->left);
		ResetVisited(cur->right);

		cur->visited = 0;
	}
}

template<typename T>
void aatree<T>::VisitBefore(T* info, typename aatree<T>::aa_node* cur)
{
	if (!cur)
		cur = cur_visited = root;

	if (cur != null_node)
	{
		VisitBefore(info, cur->left);
		if (cur->info != info)
		{
			VisitBefore(info, cur->right);
			cur->visited = 1;
		}
	}
}

template<typename T>
T* aatree<T>::GetNextInOrder(char* str)
{
	if (cur_visited == null_node)
		return NULL;

	if (cur_visited->left != null_node && cur_visited->left->visited == 0)
	{
		cur_visited = cur_visited->left;
		return GetNextInOrder(str);
	}

	if (cur_visited->visited == 0)
	{
		cur_visited->visited = 1;
		if (str)
			strcpy(str, cur_visited->name);
		return cur_visited->info;
	}

	if (cur_visited->right != null_node && cur_visited->right->visited == 0)
	{
		cur_visited = cur_visited->right;
		return GetNextInOrder(str);
	}

	cur_visited = cur_visited->parent;

	return GetNextInOrder(str);
}

template<typename T>
int aatree<T>::Remove(char* name, typename aatree<T>::aa_node** ppOutNode, int delete_info, T* info)
{
	static aa_node* last_ptr;
	static aa_node* del_ptr;
	static int item_found = 0;

	if (*ppOutNode != null_node)
	{
		last_ptr = *ppOutNode;
		if (strcmp(name, (*ppOutNode)->name) >= 0)
		{
			del_ptr = *ppOutNode;
			Remove(name, &(*ppOutNode)->right, delete_info, info);
		}
		else
		{
			Remove(name, &(*ppOutNode)->left, delete_info, info);
		}

		if (*ppOutNode == last_ptr)
		{
			if (del_ptr == null_node
				|| strcmp(name, del_ptr->name)
				|| info && del_ptr->info != info)
			{
				item_found = false;
			}
			else
			{
				delete[] del_ptr->name;
				if (del_ptr != *ppOutNode)
				{
					del_ptr->name = new char[strlen((*ppOutNode)->name) + 1];
					strcpy(del_ptr->name, (*ppOutNode)->name);
					if (delete_info)
						delete[] del_ptr->info;

					del_ptr->info_array_size = (*ppOutNode)->info_array_size;
					del_ptr->info = (*ppOutNode)->info;
				}
				*ppOutNode = (*ppOutNode)->right;
				del_ptr = null_node;
				delete last_ptr;
				--num_nodes;
				item_found = true;
			}
		}
		else if ((*ppOutNode)->left->level < (*ppOutNode)->level - 1 || (*ppOutNode)->right->level < (*ppOutNode)->level - 1)
		{
			--(*ppOutNode)->level;
			if ((*ppOutNode)->right->level > (*ppOutNode)->level)
				(*ppOutNode)->right->level = (*ppOutNode)->level;
			Skew(ppOutNode);
			Skew(&(*ppOutNode)->right);
			Skew(&(*ppOutNode)->right->right);
			Split(ppOutNode);
			Split(&(*ppOutNode)->right);
		}

	}

	return 1;
}

template<typename T>
void aatree<T>::FreeTree(typename aatree<T>::aa_node** ppOutNode, int delete_info)
{
	if (*ppOutNode == null_node)
		return;

	FreeTree(&(*ppOutNode)->left, delete_info);
	FreeTree(&(*ppOutNode)->right, delete_info);

	delete[] (*ppOutNode)->name;
	--num_nodes;

	if (delete_info)
		delete[] (*ppOutNode)->info;

	delete *ppOutNode;
	*ppOutNode = null_node;
}

