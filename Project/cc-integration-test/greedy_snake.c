#include "data_structure.c"

enum Direction {
    UNKNOWN,
    UP = 1,
    LEFT = 7,
    DOWN = 4,
    RIGHT = 13
};

enum GameState {
    RUNNING,
    QUIT
};

struct Food {
    int x, y;
};

struct Snake {
    int x, y;
    int dir;
    int ate;
    struct DList * body;
};

struct Board {
    int width, size;
    struct Vector * d;
};

void SpawnFood(struct Board * board, struct Food * food)
{
    int rx, ry;
    int w;

    w = board->width;

    rx = Random() % (w - 2); rx = rx + 1; food->x = rx;
    ry = Random() % (w - 2); ry = ry + 1; food->y = ry;

    return;
}
void SpawnSnake(struct Board * board, struct Snake * snake)
{
    struct DList * l;

    snake->x = 3;
    snake->y = 3;
    snake->dir = RIGHT;
    snake->ate = 0;

    l = Create_DList();
    snake->body = l;
    
    return;
}
void KillSnake(struct Snake * snake)
{
    Destroy_DList(snake->body);

    return;
}

void ChangeSnakeDir(struct Snake * snake, int dir)
{
    int curDir;

    curDir = snake->dir;
    if (curDir != dir && (curDir & dir) != 0)
    {
        snake->dir = dir;
    }

    return;
}
void GrowSnake(struct Snake * snake)
{
    snake->ate = snake->ate + 1;

    return;
}

void MoveSnake(struct Board * board, struct Snake * snake)
{
    int dir;
    int w;
    int revDir;

    dir = snake->dir;
    w = board->width;

    if (dir == UP && snake->y < w - 1)          snake->y = snake->y + 1;
    else if (dir == DOWN && snake->y > 0)       snake->y = snake->y - 1;
    else if (dir == RIGHT && snake->x < w - 1)  snake->x = snake->x + 1;
    else if (dir == LEFT && snake->x > 0)       snake->x = snake->x - 1;
    
    if (dir == UP || dir == DOWN) revDir = 5 - dir;
    else revDir = 20 - dir;

    DList_PushFront(snake->body, revDir);

    if (snake->ate > 0)
        snake->ate = snake->ate - 1;
    else
        DList_PopBack(snake->body);

    return;
}
int  ShouldDie(struct Board * board, struct Snake * snake)
{
    int hx, hy;
    int e;

    struct DListNode * p;
    int dir;
    int x, y;

    int die;

    hx = snake->x;
    hy = snake->y;
    e = board->width - 1;
    
    /* hit wall */
    if (hx == 0 || hx == e || hy == 0 || hy == e)
    {
        return 1;
    }

    x = hx;
    y = hy;
    for (p = DList_Begin(snake->body);
         p != DList_End(snake->body);
         p = DList_Next(snake->body, p))
    {
        dir = DList_Get(p);
        if (dir == UP)          ++y;
        else if (dir == DOWN)   --y;
        else if (dir == LEFT)   --x;
        else if (dir == RIGHT)  ++x;

        /* hit self */
        if (x == hx && y == hy)
        {
            return 1;
        }
    }

    return 0;
}
int  CanEat(struct Snake * snake, struct Food * food)
{
    int x1, y1;
    int x2, y2;

    x1 = snake->x;
    y1 = snake->y;

    x2 = food->x;
    y2 = food->y;

    if (x1 == x2 && y1 == y2)
    {
        return 1;
    }

    return 0;
}

struct Board *  CreateBoard(int width)
{
    struct Board * board;
    struct Vector * v;

    board = (struct Board *)Alloc(sizeof(struct Board));
    board->width = width;
    board->size = width * width;
    
    v = Create_Vector();
    Vector_Resize(v, width * width, ' ');
    board->d = v;

    return board;
}
void            DestroyBoard(struct Board * board)
{
    Destroy_Vector(board->d);
    Free((void *)board);

    return;
}

void ClearBoard(struct Board * board)
{
    int size;
 
    size = board->size;
    Vector_SetRange(board->d, 0, size, ' ');

    return;
}
void DrawWalls(struct Board * board)
{
    int * d;
    int i, j, w;

    w = board->width;
    d = Vector_Begin(board->d);
    { *d = '+'; ++d; } for (i = 0; i + 2 < w; ++i) { *d = '-'; ++d; } { *d = '+'; ++d; }
    for (j = 0; j + 2 < w; ++j)
    {
        { *d = '|'; ++d; } for (i = 0; i + 2 < w; ++i) { *d = ' '; ++d; } { *d = '|'; ++d; }
    }
    { *d = '+'; ++d; } for (i = 0; i + 2 < w; ++i) { *d = '-'; ++d; } { *d = '+'; ++d; }

    return;
}
void DrawSnake(struct Board * board, struct Snake * snake)
{
    int r, c;
    int w;

    struct DListNode * p;
    int dir;
    
    w = board->width;
    c = snake->x;
    r = w - 1 - snake->y;

    Vector_Set(board->d, r * w + c, 'O');
    for (p = DList_Begin(snake->body);
         p != DList_End(snake->body);
         p = DList_Next(snake->body, p))
    {
        dir = DList_Get(p);
        if (dir == UP)          --r;
        else if (dir == DOWN)   ++r;
        else if (dir == LEFT)   --c;
        else if (dir == RIGHT)  ++c;
        Vector_Set(board->d, r * w + c, 'o');
    }

    return;
}
void DrawFood(struct Board * board, struct Food * food)
{
    int r, c;
    int w;

    w = board->width;
    c = food->x;
    r = w - 1 - food->y;

    Vector_Set(board->d, r * w + c, 'X');

    return;
}
void DrawBoard(struct Board * board, int gameScore)
{
    int i, j, c, width;
    struct Vector * d;

    width = board->width;
    d = board->d;

    /*StdPrintf("size = %d x %d\n", width, width);*/

    for (i = 0; i < width; ++i)
    {
        for (j = 0; j < width; ++j)
        {
            c = Vector_Get(d, i * width + j);
            StdPrintf("%c", c);
        }

        if (i == 0)
        {
            StdPrintf(" score: %d", gameScore);
        }
        StdPrintf("\n");
    }

    return;
}

int gGameState;
int gGameScore;
int gDir;
int gFPS;

void KeyboardEventProc(int isKeyDown, unsigned int keyCode)
{
    if (isKeyDown != 0)
    {
        if (keyCode == 'W')      gDir = UP;
        else if (keyCode == 'A') gDir = LEFT;
        else if (keyCode == 'S') gDir = DOWN;
        else if (keyCode == 'D') gDir = RIGHT;
        else if (keyCode == 'Q') gGameState = QUIT;
        else StdPrintf("key '%c' pressed.\n", keyCode);
    }

    return;
}

struct Board * gBoard;
struct Snake gSnake;
struct Food gFood;

void StartGame(int size)
{
    StdPrintf("Game start!\n");

    gGameState = RUNNING;
    gGameScore = 0;
    gFPS = 3;
    gDir = UNKNOWN;
    gBoard = CreateBoard(size);
    SpawnSnake(gBoard, &gSnake);
    SpawnFood(gBoard, &gFood);

    return;
}
void UpdateScene()
{
    ChangeSnakeDir(&gSnake, gDir);
    MoveSnake(gBoard, &gSnake);

    if (ShouldDie(gBoard, &gSnake) == 1)
    {
        gGameState = QUIT;
        return;
    }

    while (CanEat(&gSnake, &gFood) == 1)
    {
        gGameScore = gGameScore + 100;
        GrowSnake(&gSnake);
        SpawnFood(gBoard, &gFood);
    }

    return;
}
void RenderNextFrame()
{
    ClearBoard(gBoard);
    DrawWalls(gBoard);
    DrawSnake(gBoard, &gSnake);
    DrawFood(gBoard, &gFood);
    DrawBoard(gBoard, gGameScore);

    return;
}
int  QuitGame()
{
    if (gGameState == QUIT)
        return 1;
    else
        return 0;
}
void StopGame()
{
    KillSnake(&gSnake);
    DestroyBoard(gBoard);
    StdPrintf("Game over. Final score = %d\n", gGameScore);

    return;
}

int main()
{
    Console_SetKeyboardEventProc(KeyboardEventProc);
    Console_EnterWindowMode();

    StartGame(10);

    while (QuitGame() != 1)
    {
        if (Console_HasMessage() != 0)
        {
            if (Console_GetMessage() != -1)
                Console_DispatchMessage();
            else
                break;
        }

        UpdateScene();
        RenderNextFrame();

        ThreadSleep(1000 / gFPS);
    }

    StopGame();

    Console_ExitWindowMode();

    return 0;
}
