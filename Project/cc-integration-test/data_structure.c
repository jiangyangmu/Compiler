#include "api.h"

/* error handling */
void MyError(const char * msg)
{
    StdPrintf("[ERROR] %s\n", msg);
    ProcExit(0);
    return;
}

/* singly linked list */
struct SList {
    struct SListNode * first;
    struct SListNode * last;
    int size;
};
struct SListNode {
    struct SListNode * next;
    int value;
};

int Print_SList(struct SList * list)
{
    struct SListNode * node;
    struct SListNode * first, * last;

    StdPrintf("SList size = %d, elements = [", list->size);
    for (node = list->first;
         node != (struct SListNode *)(long)0;
         node = node->next)
    {
        StdPrintf("%d, ", node->value);
    }
    StdPrintf("]");
    if (list->size > 0)
    {
        first = list->first;
        last = list->last;
        StdPrintf(", first = 0x%016p, last = 0x%016p", first, last);
    }
    StdPrintf("\n");

    return 0;
}

struct SList * Create_SList()
{
    struct SList * list;
    list = (struct SList *)Alloc(sizeof(struct SList));
    list->first = (struct SListNode *)(long)0;
    list->last = (struct SListNode *)(long)0;
    list->size = 0;
    return list;
}
int Destroy_SList(struct SList * list)
{
    int i;
    struct SListNode * curr, *next;

    next = list->first;
    for (i = 0; i < list->size; ++i)
    {
        curr = next;
        next = next->next;
        Free(curr);
    }
    Free(list);

    return 0;
}

int SList_Size(struct SList * list)
{
    return list->size;
}

int SList_PushFront(struct SList * list, int val)
{
    struct SListNode * node;
    struct SListNode * p;

    node = (struct SListNode *)Alloc(sizeof(struct SListNode));
    node->value = val;

    p = list->first;
    node->next = p;
    list->first = node;
    if (list->size == 0)
    {
        list->last = node;
    }
    list->size = list->size + 1;

    return 0;
}
int SList_PopFront(struct SList * list)
{
    struct SListNode * node;
    struct SListNode * p;
    int val;

    if (list->size == 0)
    {
        return 0;
    }
    else if (list->size == 1)
    {
        node = list->first;

        list->first = (struct SListNode *)(long)0;
        list->last = (struct SListNode *)(long)0;

        val = node->value;
        Free(node);

        list->size = list->size - 1;

        return val;
    }
    else
    {
        node = list->first;

        p = node->next;
        list->first = p;

        val = node->value;
        Free(node);

        list->size = list->size - 1;

        return val;
    }
}
int SList_PushBack(struct SList * list, int val)
{
    struct SListNode * node;
    struct SListNode * p;

    node = (struct SListNode *)Alloc(sizeof(struct SListNode));
    node->value = val;
    node->next = (struct SListNode *)(long)0;

    if (list->size == 0)
    {
        list->first = node;
        list->last = node;
    }
    else
    {
        p = list->last;
        p->next = node;
        list->last = node;
    }
    list->size = list->size + 1;

    return 0;
}
int SList_PopBack(struct SList * list)
{
    struct SListNode * node;
    struct SListNode * p, *q;
    int val;

    if (list->size == 0)
    {
        return 0;
    }
    else if (list->size == 1)
    {
        node = list->last;

        list->first = (struct SListNode *)(long)0;
        list->last = (struct SListNode *)(long)0;

        val = node->value;
        Free(node);

        list->size = list->size - 1;

        return val;
    }
    else
    {
        node = list->last;

        p = list->first;
        q = list->last;
        while (p->next != q)
        {
            p = p->next;
        }
        p->next = (struct SListNode *)(long)0;
        list->last = p;

        val = node->value;
        Free(node);

        list->size = list->size - 1;

        return val;
    }
}

struct SListNode * SList_Begin(struct SList * list)
{
    return list->first;
}
struct SListNode * SList_Next(struct SListNode * it)
{
    if (it != (struct SListNode *)(long)0)
        return it->next;
    else
        return (struct SListNode *)(long)0;
}
struct SListNode * SList_End(struct SList * list)
{
    return (struct SListNode *)(long)0;
}
struct SListNode * SList_Find(struct SList * list, int val)
{
    struct SListNode * node;

    for (node = SList_Begin(list);
         node != SList_End(list);
         node = SList_Next(node))
    {
        if (node->value == val)
            break;
    }

    return node;
}
int SList_InsertAfter(struct SList * list, struct SListNode * curr, int val)
{
    struct SListNode * node;
    struct SListNode * next;

    /* assert curr != 0 */

    next = curr->next;

    node = (struct SListNode *)Alloc(sizeof(struct SListNode));
    node->value = val;

    curr->next = node;
    node->next = next;

    list->size = list->size + 1;

    return 0;
}
int SList_RemoveAfter(struct SList * list, struct SListNode * curr)
{
    struct SListNode * node;
    struct SListNode * next;

    /* assert curr->next != 0 */

    node = curr->next;

    next = node;
    if (next != (struct SListNode *)(long)0)
        next = next->next;
    else
        next = (struct SListNode *)(long)0;

    Free(node);

    curr->next = next;

    list->size = list->size - 1;

    return 0;
}

int Test_SList()
{
    struct SList * list;
    struct SListNode * node;
    int i, val;

    list = Create_SList();

    Print_SList(list);

    for (i = 1; i <= 4; ++i)
    {
        SList_PushFront(list, i);
        Print_SList(list);
    }
    for (i = 5; i <= 8; ++i)
    {
        SList_PushBack(list, i);
        Print_SList(list);
    }

    StdPrintf("Iteration ");
    for (node = SList_Begin(list);
         node != SList_End(list);
         node = SList_Next(node))
    {
        StdPrintf("%d, ", node->value);
    }
    StdPrintf("\n");

    node = SList_Find(list, 5);
    StdPrintf("Find 5 = %d\n", node->value);

    SList_InsertAfter(list, node, 100);
    Print_SList(list);
    SList_RemoveAfter(list, node);
    Print_SList(list);

    for (i = 0; i < 4; ++i)
    {
        val = SList_PopFront(list);
        StdPrintf("PopFront = %d\n", val);
        Print_SList(list);
    }
    for (i = 4; i < 8; ++i)
    {
        val = SList_PopBack(list);
        StdPrintf("PopBack = %d\n", val);
        Print_SList(list);
    }

    Destroy_SList(list);

    return 0;
}

/* doubly linked list */
struct DList {
    struct DListNode * guard;
    int size;
};
struct DListNode {
    struct DListNode * prev;
    struct DListNode * next;
    int value;
};

int Print_DList(struct DList * list)
{
    struct DListNode * guard;
    struct DListNode * node;

    guard = list->guard;

    StdPrintf("DList size = %d, elements = [", list->size);
    for (node = guard->next;
         node != guard;
         node = node->next)
    {
        StdPrintf("%d, ", node->value);
    }
    StdPrintf("]\n");

    return 0;
}

struct DList * Create_DList()
{
    struct DList * list;
    struct DListNode * guard;

    list = (struct DList *)Alloc(sizeof(struct DList));
    list->size = 0;

    guard = (struct DListNode *)Alloc(sizeof(struct DListNode));
    guard->prev = guard;
    guard->next = guard;
    guard->value = 0;
    list->guard = guard;

    return list;
}
int Destroy_DList(struct DList * list)
{
    struct DListNode * guard;
    struct DListNode * curr, *next;

    guard = list->guard;
    next = guard->next;
    while (next != guard)
    {
        curr = next;
        next = next->next;
        Free(curr);
    }
    Free(guard);
    Free(list);

    return 0;
}

int DList_Size(struct DList * list)
{
    return list->size;
}

int DList_PushFront(struct DList * list, int val)
{
    struct DListNode * node;
    struct DListNode * curr, *next;

    node = (struct DListNode *)Alloc(sizeof(struct DListNode));
    node->value = val;

    curr = list->guard;
    next = curr->next;

    node->next = next;
    node->prev = curr;

    curr->next = node;
    next->prev = node;

    list->size = list->size + 1;

    return 0;
}
int DList_PopFront(struct DList * list)
{
    struct DListNode * node;
    struct DListNode * curr, *next;
    int val;

    if (list->size == 0)
    {
        return 0;
    }
    else
    {
        curr = list->guard;
        node = curr->next;
        next = node->next;

        curr->next = next;
        next->prev = curr;

        val = node->value;
        Free(node);

        list->size = list->size - 1;

        return val;
    }
}
int DList_PushBack(struct DList * list, int val)
{
    struct DListNode * node;
    struct DListNode * curr, *next;

    node = (struct DListNode *)Alloc(sizeof(struct DListNode));
    node->value = val;

    next = list->guard;
    curr = next->prev;

    node->next = next;
    node->prev = curr;

    curr->next = node;
    next->prev = node;

    list->size = list->size + 1;

    return 0;
}
int DList_PopBack(struct DList * list)
{
    struct DListNode * node;
    struct DListNode * curr, *next;
    int val;

    if (list->size == 0)
    {
        return 0;
    }
    else
    {
        next = list->guard;
        node = next->prev;
        curr = node->prev;

        curr->next = next;
        next->prev = curr;

        val = node->value;
        Free(node);

        list->size = list->size - 1;

        return val;
    }
}

/* TODO: add test */
struct DListNode * DList_Begin(struct DList * list)
{
    struct DListNode * n;
    n = list->guard;
    n = n->next;
    return n;
}
struct DListNode * DList_End(struct DList * list)
{
    return list->guard;
}
struct DListNode * DList_Next(struct DList * list, struct DListNode * p)
{
    return p->next;
}
int                DList_Get(struct DListNode * p)
{
    return p->value;
}

int Test_DList()
{
    struct DList * list;
    int i;

    list = Create_DList();

    Print_DList(list);

    for (i = 1; i <= 4; ++i)
    {
        DList_PushFront(list, i);
        Print_DList(list);
    }
    for (i = 5; i <= 8; ++i)
    {
        DList_PushBack(list, i);
        Print_DList(list);
    }

    Destroy_DList(list);

    return 0;
}

/* vector */
struct Vector {
    int * data;
    int capacity;
    int size;
};

int Print_Vector(struct Vector * vec)
{
    int i;

    StdPrintf("Vector capacity = %d, size = %d, elements = [",
              vec->capacity, vec->size);
    for (i = 0; i < vec->size; ++i)
    {
        StdPrintf("%d, ", vec->data[i]);
    }
    StdPrintf("]\n");

    return 0;
}

struct Vector * Create_Vector()
{
    struct Vector * vec;

    vec = (struct Vector *)Alloc(sizeof(struct Vector));
    vec->capacity = 0;
    vec->data = (int *)(long)0;
    vec->size = 0;

    return vec;
}
int Destroy_Vector(struct Vector * vec)
{
    if (vec->data != (int *)(long)0)
    {
        Free(vec->data);
    }
    Free(vec);

    return 0;
}

int Vector_Size(struct Vector * vec)
{
    return vec->size;
}
int Vector_Capacity(struct Vector * vec)
{
    return vec->capacity;
}

int __Vector_Expand(struct Vector * vec, int capacity)
{
    int * data;
    int i;
    int val;

    if (vec->capacity < capacity)
    {
        data = (int *)Alloc(sizeof(int) * capacity);

        if (vec->data != (int *)(long)0)
        {
            for (i = 0; i < vec->size; ++i)
            {
                val = vec->data[i];
                data[i] = val;
            }
            Free(vec->data);
        }
        vec->data = data;
        vec->capacity = capacity;
    }

    return 0;
}
int Vector_PushBack(struct Vector * vec, int val)
{
    int capacity;
    int i;

    capacity = vec->capacity;
    if (vec->size >= capacity)
    {
        __Vector_Expand(vec, 16 + vec->size * 2);
    }

    i = vec->size;
    vec->data[i] = val;
    vec->size = vec->size + 1;

    return 0;
}
int Vector_PopBack(struct Vector * vec)
{
    int val;
    int i;

    vec->size = vec->size - 1;
    i = vec->size;
    val = vec->data[i];

    return val;
}
/* TODO: add test */
int Vector_Resize(struct Vector * vec, int newSize, int fill)
{
    int i;

    if (vec->capacity < newSize)
    {
        __Vector_Expand(vec, newSize);
    }
    
    for (i = vec->size; i < newSize; ++i)
    {
        vec->data[i] = fill;
    }

    vec->size = newSize;

    return 0;
}

int Vector_Get(struct Vector * vec, int index)
{
    /* assert 0 <= index < size */
    return vec->data[index];
}
int Vector_Set(struct Vector * vec, int index, int val)
{
    /* assert 0 <= index < size */
    vec->data[index] = val;
    return 0;
}
int Vector_SetRange(struct Vector * vec, int begin, int end, int val)
{
    /* assert 0 <= begin <= end <= size */
    int i;

    for (i = begin; i < end; ++i)
    {
        vec->data[i] = val;
    }

    return 0;
}

int * Vector_Begin(struct Vector * vec)
{
    return vec->data;
}
int * Vector_End(struct Vector * vec)
{
    int size;
    size = vec->size;
    if (vec->data != (int *)(long)0)
        return vec->data + size;
    else
        return (int *)(long)0;
}
int * Vector_Next(struct Vector * vec, int * p)
{
    if (p != (int *)(long)0)
        return p + 1;
    else
        return (int *)(long)0;
}
int * Vector_Find(struct Vector * vec, int val)
{
    int * begin, *end;
    int * p;

    begin = vec->data;
    end = begin + vec->size;
    for (p = begin; p != end; ++p)
    {
        if (*p == val)
            break;
    }

    return p;
}

int Test_Vector()
{
    struct Vector * vec;
    int * p;
    int i;

    vec = Create_Vector();

    Print_Vector(vec);

    for (i = 1; i <= 12; ++i)
    {
        Vector_PushBack(vec, i);
        Print_Vector(vec);
    }
    Vector_PopBack(vec);
    Print_Vector(vec);
    Vector_PopBack(vec);
    Print_Vector(vec);

    StdPrintf("Get ");
    for (i = 0; i < 10; ++i)
    {
        StdPrintf("%d, ", Vector_Get(vec, i));
    }
    StdPrintf("\nIteration ");
    for (p = Vector_Begin(vec);
         p != Vector_End(vec);
         p = Vector_Next(vec, p))
    {
        StdPrintf("%d, ", *p);
    }
    StdPrintf("\nFind ");
    for (i = 5; i < 15; ++i)
    {
        StdPrintf("%d = ", i);
        if (Vector_Find(vec, i) != Vector_End(vec))
            StdPrintf("true, ");
        else
            StdPrintf("false, ");
    }
    StdPrintf("\n");

    StdPrintf("Set * 2 => ");
    for (i = 0; i < 10; ++i)
    {
        Vector_Set(vec, i, i * 2);
    }
    Print_Vector(vec);

    StdPrintf("SetRange 100 => ");
    Vector_SetRange(vec, 0, 10, 100);
    Print_Vector(vec);

    Destroy_Vector(vec);

    return 0;
}

/* tree set */
struct TreeSet {
    struct TreeSetNode * root;
    int size;
};

struct TreeSetNode {
    struct TreeSetNode * left;
    struct TreeSetNode * right;
    int value;
};

int __Print_TreeSetImpl(struct TreeSetNode * node)
{
    if (node != (struct TreeSetNode *)(long)0)
    {
        StdPrintf("(");
        __Print_TreeSetImpl(node->left);
        StdPrintf(", %d, ", node->value);
        __Print_TreeSetImpl(node->right);
        StdPrintf(")");
    }

    return 0;
}
int Print_TreeSet(struct TreeSet * set)
{
    StdPrintf("TreeSet size = %d, elements = [", set->size);
    __Print_TreeSetImpl(set->root);
    StdPrintf("]\n");
    return 0;
}

struct TreeSet * Create_TreeSet()
{
    struct TreeSet * set;

    set = (struct TreeSet *)Alloc(sizeof(struct TreeSet));
    set->root = (struct TreeSetNode *)(long)0;
    set->size = 0;

    return set;
}

int __Destroy_TreeSetImpl(struct TreeSetNode * node)
{
    if (node != (struct TreeSetNode *)(long)0)
    {
        __Destroy_TreeSetImpl(node->left);
        __Destroy_TreeSetImpl(node->right);
        Free(node);
    }

    return 0;
}
int Destroy_TreeSet(struct TreeSet * set)
{
    __Destroy_TreeSetImpl(set->root);
    Free(set);

    return 0;
}

int __TreeSet_InsertImpl(struct TreeSetNode ** p, int val)
{
    struct TreeSetNode * node;
    int count;

    if (*p == (struct TreeSetNode *)(long)0)
    {
        node = (struct TreeSetNode *)Alloc(sizeof(struct TreeSetNode));
        node->left = (struct TreeSetNode *)(long)(0);
        node->right = (struct TreeSetNode *)(long)(0);
        node->value = val;

        *p = node;
        count = 1;
    }
    else
    {
        if ((*p)->value < val)
            count = __TreeSet_InsertImpl(&((*p)->right), val);
        else if (val < (*p)->value)
            count = __TreeSet_InsertImpl(&((*p)->left), val);
        else
            count = 0;
    }

    return count;
}
int TreeSet_Insert(struct TreeSet * set, int val)
{
    int count;

    count = __TreeSet_InsertImpl(&(set->root), val);
    set->size = set->size + count;

    return 0;
}

struct TreeSetNode * __TreeSet_MoveLeftUp(struct TreeSetNode ** p)
{
    struct TreeSetNode * node;
    struct TreeSetNode * left, *right;

    /* assert node != 0 */

    node = *p;
    left = node->left;
    right = node->right;

    *p = left;
    if (left != (struct TreeSetNode *)(long)0)
        left->right = right;

    return node;
}
struct TreeSetNode * __TreeSet_MoveRightUp(struct TreeSetNode ** p)
{
    struct TreeSetNode * node;
    struct TreeSetNode * left, *right;

    /* assert node != 0 */

    node = *p;
    right = node->right;
    left = node->left;

    *p = right;
    if (right != (struct TreeSetNode *)(long)0)
        right->left = left;

    return node;
}
struct TreeSetNode * __TreeSet_Replace(struct TreeSetNode ** p, struct TreeSetNode * newNode)
{
    struct TreeSetNode * oldNode;
    struct TreeSetNode * left, *right;

    /* assert newNode is orphan */

    oldNode = *p;
    left = oldNode->left;
    right = oldNode->right;

    newNode->left = left;
    newNode->right = right;
    *p = newNode;

    return oldNode;
}
int __TreeSet_RemoveImpl(struct TreeSetNode ** p, int val)
{
    struct TreeSetNode ** prev, ** curr, ** next;
    struct TreeSetNode * prevNode, *currNode, *nextNode;
    int count;

    if (*p != (struct TreeSetNode *)(long)0)
    {
        if ((*p)->value < val)
            count = __TreeSet_RemoveImpl(&((*p)->right), val);
        else if (val < (*p)->value)
            count = __TreeSet_RemoveImpl(&((*p)->left), val);
        else
        {
            curr = p;
            prev = &((*curr)->left);
            next = &((*curr)->right);
            if (*prev != (struct TreeSetNode *)(long)0)
            {
                while ((*prev)->right != (struct TreeSetNode *)(long)0)
                {
                    prev = &((*prev)->right);
                }

                prevNode = __TreeSet_MoveLeftUp(prev);
                currNode = __TreeSet_Replace(curr, prevNode);
                Free(currNode);
            }
            else if (*next != (struct TreeSetNode *)(long)0)
            {
                while ((*next)->left != (struct TreeSetNode *)(long)0)
                {
                    next = &((*next)->left);
                }

                nextNode = __TreeSet_MoveRightUp(next);
                currNode = __TreeSet_Replace(curr, nextNode);
                Free(currNode);
            }
            else
            {
                Free(*p);
                *p = (struct TreeSetNode *)(long)0;
            }

            count = 1;
        }
    }
    else
    {
        count = 0;
    }

    return count;
}
int TreeSet_Remove(struct TreeSet * set, int val)
{
    int count;

    count = __TreeSet_RemoveImpl(&(set->root), val);
    set->size = set->size - count;

    return 0;
}

struct TreeSetNode * TreeSet_End(struct TreeSet * set)
{
    return (struct TreeSetNode *)(long)(0);
}
struct TreeSetNode * __TreeSet_FindImpl(struct TreeSetNode * node, int val)
{
    struct TreeSetNode * result;

    if (node != (struct TreeSetNode *)(long)(0))
    {
        if (node->value < val)
            result = __TreeSet_FindImpl(node->right, val);
        else if (val < node->value)
            result = __TreeSet_FindImpl(node->left, val);
        else
            result = node;
    }
    else
    {
        result = (struct TreeSetNode *)(long)(0);
    }

    return result;
}
struct TreeSetNode * TreeSet_Find(struct TreeSet * set, int val)
{
    return __TreeSet_FindImpl(set->root, val);
}

int Test_TreeSet()
{
    struct TreeSet * set;
    int i;

    set = Create_TreeSet();

    Print_TreeSet(set);

    for (i = 0; i < 10; ++i)
    {
        TreeSet_Insert(set, i + 1);
        Print_TreeSet(set);
    }
    for (i = 0; i < 10; ++i)
    {
        TreeSet_Insert(set, i + 1);
        Print_TreeSet(set);
    }
    StdPrintf("Find ");
    for (i = 5; i < 15; ++i)
    {
        StdPrintf("%d = ", i);
        if (TreeSet_Find(set, i) != TreeSet_End(set))
            StdPrintf("true, ");
        else
            StdPrintf("false, ");
    }
    StdPrintf("\n");
    for (i = 0; i < 10; ++i)
    {
        TreeSet_Remove(set, i + 1);
        Print_TreeSet(set);
    }

    Destroy_TreeSet(set);

    set = Create_TreeSet();

    Print_TreeSet(set);

    for (i = 0; i < 10; ++i)
    {
        TreeSet_Insert(set, 10 - i);
        Print_TreeSet(set);
    }
    for (i = 0; i < 10; ++i)
    {
        TreeSet_Insert(set, 10 - i);
        Print_TreeSet(set);
    }
    for (i = 0; i < 10; ++i)
    {
        TreeSet_Remove(set, 10 - i);
        Print_TreeSet(set);
    }

    Destroy_TreeSet(set);

    set = Create_TreeSet();
    TreeSet_Insert(set, 5);
    TreeSet_Insert(set, 1);
    TreeSet_Insert(set, 6);
    TreeSet_Insert(set, 3);
    TreeSet_Insert(set, 4);
    TreeSet_Insert(set, 10);
    TreeSet_Insert(set, 8);
    TreeSet_Insert(set, 2);
    TreeSet_Insert(set, 7);
    TreeSet_Insert(set, 9);
    Print_TreeSet(set);
    StdPrintf("Find ");
    for (i = 5; i < 15; ++i)
    {
        StdPrintf("%d = ", i);
        if (TreeSet_Find(set, i) != TreeSet_End(set))
            StdPrintf("true, ");
        else
            StdPrintf("false, ");
    }
    StdPrintf("\n");
    for (i = 0; i < 10; ++i)
    {
        TreeSet_Remove(set, 10 - i);
        Print_TreeSet(set);
    }
    Destroy_TreeSet(set);

    return 0;
}

/* hash set */
struct HashSet {
    struct HashSetBucket * buckets;
    int totalCount;
    int usedCount;
    int size;
};

struct HashSetBucket {
    struct Vector * data;
};

int Print_HashSet(struct HashSet * set)
{
    struct HashSetBucket * bucket;
    struct Vector * bucketData;
    int bucketIndex;

    StdPrintf("HashSet size = %d, total buckets = %d, ",
              set->size,
              set->totalCount);
    StdPrintf("used buckets = %d, buckets = [\n",
              set->usedCount);
    for (bucketIndex = 0; bucketIndex < set->totalCount; ++bucketIndex)
    {
        bucket = set->buckets + bucketIndex;
        bucketData = bucket->data;
        if (bucketData->size > 0)
        {
            StdPrintf("  %d: ", bucketIndex);
            Print_Vector(bucketData);
        }
    }
    StdPrintf("]\n");

    return 0;
}

struct HashSet * Create_HashSet()
{
    struct HashSet * set;

    set = (struct HashSet *)Alloc(sizeof(struct HashSet));
    set->buckets = (struct HashSetBucket *)(long)0;
    set->totalCount = 0;
    set->usedCount = 0;
    set->size = 0;

    return set;
}

int Destroy_HashSet(struct HashSet * set)
{
    struct Vector * bucketData;
    int bucketIndex;

    if (set->buckets != (struct HashSetBucket *)(long)0)
    {
        for (bucketIndex = 0; bucketIndex < set->totalCount; ++bucketIndex)
        {
            bucketData = (set->buckets + bucketIndex)->data;
            Destroy_Vector(bucketData);
        }

        Free(set->buckets);
    }
    Free(set);

    return 0;
}

int __HashSet_Hash(struct HashSet * set, int val)
{
    int hash;

    if (set->totalCount <= 0)
        MyError("__HashSet_Hash: bad hash set.");

    hash = (val * 37) % set->totalCount;
    hash = hash + set->totalCount;
    hash = hash % set->totalCount;

    if (hash < 0)
        MyError("__HashSet_Hash: bad hash.");

    return hash;
}
int __HashSet_Swap(struct HashSet * left, struct HashSet * right)
{
    struct HashSetBucket * buckets1, *buckets2;
    int i1, i2;

    buckets1 = left->buckets;
    buckets2 = right->buckets;
    left->buckets = buckets2;
    right->buckets = buckets1;

    i1 = left->size;
    i2 = right->size;
    left->size = i2;
    right->size = i1;

    i1 = left->totalCount;
    i2 = right->totalCount;
    left->totalCount = i2;
    right->totalCount = i1;

    i1 = left->usedCount;
    i2 = right->usedCount;
    left->usedCount = i2;
    right->usedCount = i1;

    return 0;
}
int HashSet_Insert(struct HashSet * set, int val);
int __HashSet_Rehash(struct HashSet * set)
{
    struct HashSet * newSet;
    struct HashSetBucket * newBuckets;
    struct HashSetBucket * newBucket;
    int newTotalCount;
    int i;

    struct HashSetBucket * bucket;
    struct Vector * bucketData;
    int bucketIndex;
    int * p;

    if (set->totalCount * 3 <= set->usedCount * 5)
    {
        newSet = Create_HashSet();
        newTotalCount = 16 + set->totalCount * 2;
        newBuckets = (struct HashSetBucket *)Alloc(sizeof(struct HashSetBucket) * newTotalCount);
        newSet->buckets = newBuckets;
        newSet->totalCount = newTotalCount;
        newSet->usedCount = 0;
        newSet->size = 0;
        for (bucketIndex = 0; bucketIndex < newTotalCount; ++bucketIndex)
        {
            newBucket = newSet->buckets + bucketIndex;
            bucketData = Create_Vector();
            newBucket->data = bucketData;
        }

        for (bucketIndex = 0; bucketIndex < set->totalCount; ++bucketIndex)
        {
            bucket = set->buckets + bucketIndex;
            bucketData = bucket->data;
            for (p = Vector_Begin(bucketData);
                 p != Vector_End(bucketData);
                 p = Vector_Next(bucketData, p))
            {
                i = *p;
                HashSet_Insert(newSet, i);
            }
        }

        __HashSet_Swap(set, newSet);

        Destroy_HashSet(newSet);
    }

    return 0;
}

int HashSet_Insert(struct HashSet * set, int val)
{
    struct Vector * bucketData;
    int bucketIndex;

    __HashSet_Rehash(set);

    bucketIndex = __HashSet_Hash(set, val);
    bucketData = (set->buckets + bucketIndex)->data;
    if (Vector_Find(bucketData, val) == Vector_End(bucketData))
    {
        Vector_PushBack(bucketData, val);
        if (bucketData->size == 1)
            set->usedCount = set->usedCount + 1;
        set->size = set->size + 1;
    }

    return 0;
}

int HashSet_Remove(struct HashSet * set, int val)
{
    struct Vector * bucketData;
    int bucketIndex;
    int * curr, * next;
    int i;

    bucketIndex = __HashSet_Hash(set, val);
    bucketData = (set->buckets + bucketIndex)->data;

    next = Vector_Find(bucketData, val);
    if (next != Vector_End(bucketData))
    {
        curr = next;
        next = Vector_Next(bucketData, next);
        while (next != Vector_End(bucketData))
        {
            i = *next;
            *curr = i;

            curr = next;
            next = Vector_Next(bucketData, next);
        }

        Vector_PopBack(bucketData);
        if (bucketData->size == 0)
            set->usedCount = set->usedCount - 1;
        set->size = set->size - 1;
    }

    return 0;
}

int Test_HashSet()
{
    struct HashSet * set;
    int i;

    set = Create_HashSet();

    Print_HashSet(set);

    for (i = 0; i < 7; ++i)
    {
        HashSet_Insert(set, i + 1);
        Print_HashSet(set);
    }

    for (i = 0; i < 7; ++i)
    {
        HashSet_Insert(set, i + 1);
    }
    Print_HashSet(set);

    for (i = 0; i < 7; ++i)
    {
        HashSet_Remove(set, i + 1);
        Print_HashSet(set);
    }

    Destroy_HashSet(set);

    return 0;
}

/*
int main()
{
    Test_SList();
    Test_DList();
    Test_Vector();
    Test_TreeSet();
    Test_HashSet();

    return 0;
}
*/
