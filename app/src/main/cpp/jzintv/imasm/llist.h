/*

Linked list template class

Copyright (C) 2003  Joe Fisher, Shiny Technologies, LLC
http://www.shinytechnologies.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef LLIST_H_INCLUDED
#define LLIST_H_INCLUDED

#include <stdlib.h>

#define LIST_OUT_OF_MEMORY -1

template <class T>
struct Node
{
    T data;
    Node *next;
};

template <class T>
class LList
{
    public:
        LList();
        ~LList();
        LList<T> &operator=(LList<T> &);

        int AddNode(T &);
        T *GetNode(int);
        T *GetFirstNode();
        T *GetLastNode();
        int FindNodeIndex(const T *);

        int DeleteNode(int);
        int DeleteFirstNode();
        int DeleteLastNode();
        int GetNumNodes() {return m_iNum;}
        void Empty();
        int IsEmpty() {return (m_iNum == 0);}

        void push(T);
        T pop();
        T peek();


    private:
        Node<T> *m_start;
        int m_iNum;

        int last_get_node;
        Node<T> *m_last_get_node;
};

template <class T>
LList<T>::LList()
{
    m_start = NULL;
    m_iNum = 0;
    m_last_get_node = NULL;
    last_get_node = -1;
}

template <class T>
LList<T>::~LList()
{
    Empty();
}

template <class T>
LList<T> &LList<T>::operator=(LList<T> &l)
{
    int i, iNum;
    T *data;

    Empty();
    iNum = l.GetNumNodes();
    for (i = 0; i < iNum; i++)
    {
        data = l.GetNode(i);
        AddNode(*data);
    }

    return *this;
}

template <class T>
int LList<T>::AddNode(T &add)
{
    Node<T> *n = m_start;
    Node<T> *newNode = NULL;

    newNode = new Node<T>;

    if (newNode == NULL)
    {
        return LIST_OUT_OF_MEMORY;
    }

    if (n != NULL)
    {
        while (n->next != NULL)
        {
            n = n->next;
        }

        n->next = newNode;
    }
    else
    {
        m_start = newNode;
    }

    newNode->data = add;

    newNode->next = NULL;

    m_iNum++;
    return 1;
}

template <class T>
T *LList<T>::GetNode(int num)
{
    Node<T> *n = m_start;
    int i = 0;

    if (num >= last_get_node && m_last_get_node != NULL)
    {
        n = m_last_get_node;
        i = last_get_node;
    }

    while (n != NULL)
    {
        if (i == num)
        {
            m_last_get_node = n;
            last_get_node = num;
            return &(n->data);
        }

        i++;
        n = n->next;
    }
    return 0;
}

template <class T>
T *LList<T>::GetFirstNode()
{
    return GetNode(0);
}

template <class T>
T *LList<T>::GetLastNode()
{
    return GetNode(m_iNum - 1);
}

template <class T>
int LList<T>::FindNodeIndex(const T *toFind)
{
    T *pItem;
    int i = 0;

    for (i = 0; i < m_iNum; i++)
    {
        pItem = GetNode(i);

        if (pItem == toFind)
        {
            return i;
        }
    }

    return -1;
}

template <class T>
int LList<T>::DeleteNode(int num)
{
    Node<T> *n = m_start;
    Node<T> *parent = NULL;
    int i = 0;

    if (num < last_get_node)
    {
        last_get_node = -1;
        m_last_get_node = NULL;
    }

    if ((IsEmpty()) || (num >= m_iNum))
    {
        return 0;
    }

    while (n != NULL)
    {
        if (i == num)
        {
            break;
        }

        i++;
        parent = n;
        n = n->next;
    }

    if (n == NULL)  // Should never happen
    {
        return 0;
    }

    if (parent == NULL)
    {
        n = m_start;
        m_start = n->next;
        delete n;
    }
    else
    {
        parent->next = n->next;
        delete n;
    }

    m_iNum--;
    return 1;
}

template <class T>
int LList<T>::DeleteFirstNode()
{
    return DeleteNode(0);
}

template <class T>
int LList<T>::DeleteLastNode()
{
    return DeleteNode(m_iNum - 1);
}

template <class T>
void LList<T>::Empty()
{
    Node<T> *n = m_start;
    Node<T> *delNode = NULL;

    while (n != NULL)
    {
        delNode = n;
        n = n->next;
        delete delNode;
    }

    m_iNum = 0;
    m_start = NULL;
    m_last_get_node = NULL;
    last_get_node = -1;
}

template <class T>
void LList<T>::push(T data)
{
    AddNode(data);
}

template <class T>
T LList<T>::pop()
{
    T data, *pData;

    pData = GetLastNode();

    if (pData != NULL)
    {
        data = *pData;
    }
    m_last_get_node = NULL;
    last_get_node = -1;

    DeleteLastNode();
    return data;
}

template <class T>
T LList<T>::peek()
{
    T data, *pData;

    pData = GetLastNode();

    if (pData != NULL)
    {
        data = *pData;
    }

    return data;
}


#endif


