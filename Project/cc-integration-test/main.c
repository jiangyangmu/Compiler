struct Node {
    int value;
    struct Node * next;
};

int main()
{
    struct Node n1, n2;
    struct Node *p;

    n1.value = 1;
    n2.value = 2;

    n1.next = &n2;

    p = &n1;

    return p->next->value;
}
