struct Node {
    struct Node * next;
    int value;
};

int main()
{
    struct Node n1, n2, n3;
    struct Node *p;

    n1.next = &n2;
    n2.next = &n3;

    n1.value = 1;
    n2.value = 2;
    n3.value = 3;

    p = &n1;
    p = p->next;
    p = p->next;

    return p->value;
}
