#pragma once

#include <deque>

struct Node {
    struct Node * up, * down, * left, * right;
    int data;
};

void ForEach(Node * node, void (*func)(Node *))
{
    if (node->down)
    {
        ForEach(node->down, func);
    }

    func(node);

    if (node->right)
    {
        ForEach(node->right, func);
    }
}

void ForEach2(Node * node, void(*func)(Node *))
{
    std::deque<std::pair<Node *, bool>> q = {{node, false}};
    while (!q.empty())
    {
        Node * n = q.back().first;
        if (q.back().second)
        {
            q.pop_back();

            func(n);

            if (n->right)
            {
                q.emplace_back(n->right, false);
            }
        }
        else
        {
            q.back().second = true;
            
            if (n->down)
            {
                q.emplace_back(n->down, false);
            }
        }
    }
}

void ReplaceAsParent(Node * node, Node * parent)
{
    // parent must be an orphan.

    parent->left = node->left;
    if (node->left) node->left->right = parent;
    node->left = nullptr;

    parent->right = node->right;
    if (node->right) node->right->left = parent;
    node->right = nullptr;

    parent->up = node->up;
    if (node->up && node->up->down == node) node->up->down = parent;

    node->up = parent;
    parent->down = node;
}
