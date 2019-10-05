/*
    Input/Output

    [File]
    Internal
        State = {position, error}
        Constant = {EOF, FILE_START, FILE_END}
    Operation
        Open, Close
        Read
        Write (override, append, both)
        SetPos, GetPos, IsEOF
    * error handling

    [Stream]
    Internal
        State = {position, error}
    Operation
        Open, Close
        Get, Put
    * error handling

    * formatting
*/
struct FileHandle;
struct FilePosition {
    unsigned long long pos;
    int eof;
};
struct FileHandle * OpenFile(const char * fileName, const char * mode);
void                CloseFile(struct FileHandle * fileHandle);
void                ReadFile(struct FileHandle * fileHandle, void * buffer, unsigned long long size);
void                WriteFile(struct FileHandle * fileHandle, void * buffer, unsigned long long size);
void                GetFilePosition(struct FileHandle * fileHandle, struct FilePosition * newPos);
void                SetFilePosition(struct FileHandle * fileHandle, const struct FilePosition * newPos);
int                 IsEOF(struct FileHandle * fileHandle);

unsigned long long  GetFileSize(struct FileHandle * fileHandle);
void                SeekFileBegin(struct FileHandle * fileHandle);
void                SeekFileEnd(struct FileHandle * fileHandle);

/*
struct InputStreamHandle;
struct OutputStreamHandle;
struct InputStreamHandle *  OpenInputStreamFromFile(struct FileHandle * fileHandle);
struct InputStreamHandle *  OpenInputStreamFromStdin();
void                        IsEOS(struct InputStreamHandle *);
struct OutputStreamHandle * OpenOutputStreamFromFile(struct FileHandle * fileHandle);
struct OutputStreamHandle * OpenOutputStreamFromStdout();
struct OutputStreamHandle * OpenOutputStreamFromStderr();
void                        SetEOS(struct OutputStreamHandle *);
void                        PutChar(struct OutputStreamHandle * outputStreamHandle, int c);
int                         GetChar(struct InputStreamHandle * inputStreamHandle);
*/

/*
    Windows Subsytem

    [Message]
    main loop

    [Device]
    Mouse
    Keyboard
    Screen

    Console Subsystem
*/

void            Console_EnterWindowMode();
int             Console_HasMessage();
int             Console_GetMessage();
void            Console_DispatchMessage();
void            Console_SetKeyboardEventProc(void(*keyboardEventProc)(int _isKeyDown, unsigned int _keyCode));
void            Console_SetMouseEventProc();
void            Console_SetWindowBufferSizeEventProc();
void            Console_ExitWindowMode();

/*
    Memory blob, C string
*/
void                MemSet(void * begin, void * end, char value);
int                 MemCmp(const void * left, const void * right, unsigned long long num);
void                MemCpy(const void * from, void * to, unsigned long long num);
void                MemMov(const void * from, void * to, unsigned long long num);

unsigned long long  CStrLen(const char * str);
int                 CStrCmp(const char * s1, const char * s2);
void                CStrCpy(const char * from, char * to);
/*
// TODO: str cat
// TODO: str start-with/end-with
// TODO: str/mem search first/last char/char-in-set/char-not-in-set, return index/pointer
// TODO: simple tokenizer
*/

/* int, float, math */
int     Random();

float   FltCeil(float f);
float   FltFloor(float f);
float   FltMod(float numer, float denom);
float   FltAbs(float f);

float   FltSin(float x);
float   FltCos(float x);
float   FltTan(float x);

float   FltLog(float x);
float   FltPow(float base, float exponent);
float   FltSqrt(float x);
/*
// TODO: int: abs, div
// TODO: flt: repr1 (sign, mantissa, exponent), repr2 (integer part, fractional part)
*/

/*
    Data Conversion

    [formatted io]
    [serialization]
    [marshalling]
    [encoding]
*/
void    StdScanf(const char * fmt, ...);
void    StdPrintf(const char * fmt, ...);
/*
// TODO: wchar_t <-> multi-byte <-> utf-8/utf-16
*/

/* memory allocation */
void *  Alloc(unsigned long int size);
int     Free(void * ptr);

/* time, clock */
long        GetClock();
long long   GetCurrentTime();

/*
    Process, Thread
    
    [error handling]
    exit, atexit
    abort

    sleep
*/
void    ProcAbort();
void    ProcExit(int status);
int     ProcAtExit(void(*func)());

void    ThreadSleep(int ms);

struct ThreadHandle;
struct ThreadHandle *   Create_Thread(unsigned long(*func)(void * _p), void * param);
void                    Join_Thread(struct ThreadHandle * threadHandle);

struct MutexHandle;
struct MutexHandle * Create_Mutex();
void Destroy_Mutex(struct MutexHandle * mutexHandle);
void Lock_Mutex(struct MutexHandle * mutexHandle);
void Unlock_Mutex(struct MutexHandle * mutexHandle);

/* assertion */
