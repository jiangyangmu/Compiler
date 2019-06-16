#include "api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>

struct FileHandle * OpenFile(const char * filename, const char * mode)
{
    FILE * file;
    errno_t e;

    e = fopen_s(&file, filename, mode);
    if (e)
    {
        printf("open  file '%s' failed, errno = %d\n", filename, e);
        exit(1);
    }
    else
    {
        printf("open  file 0x%016p, filename = '%s', mode = '%s'\n", file, filename, mode);
    }

    return (struct FileHandle *)file;
}

void CloseFile(struct FileHandle * fileHandle)
{
    int ret;

    ret = fclose((FILE *)fileHandle);
    if (ret != 0)
    {
        printf("close file 0x%016p failed\n", fileHandle);
        exit(1);
    }
    else
    {
        printf("close file 0x%016p\n", fileHandle);
    }
}

void ReadFile(struct FileHandle * fileHandle, void * buffer, unsigned long long size)
{
    size_t cnt;

    cnt = fread(buffer, 1, size, (FILE *)fileHandle);
    if (cnt != size)
    {
        printf("read  file 0x%016p, buf = 0x%016p, size = %llu, actual size = %llu\n", fileHandle, buffer, size, cnt);
        exit(1);
    }
    else
    {
        printf("read  file 0x%016p, buf = 0x%016p, size = %llu\n", fileHandle, buffer, size);
    }
}

void WriteFile(struct FileHandle * fileHandle, void * buffer, unsigned long long size)
{
    size_t cnt;

    cnt = fwrite(buffer, 1, size, (FILE *)fileHandle);
    if (cnt != size)
    {
        printf("write file 0x%016p, buf = 0x%016p, size = %llu, actual size = %llu\n", fileHandle, buffer, size, cnt);
        exit(1);
    }
    else
    {
        /* if (fflush((FILE *)fileHandle) != 0)
            printf("write file 0x%016p, buf = 0x%016p, size = %llu, flush error\n", fileHandle, buffer, size);
        else */
            printf("write file 0x%016p, buf = 0x%016p, size = %llu\n", fileHandle, buffer, size);
    }
}

void GetFilePosition(struct FileHandle * fileHandle, struct FilePosition * newPos)
{
    newPos->pos = (unsigned long long)ftell((FILE *)fileHandle);

    if (feof((FILE *)fileHandle))
        newPos->eof = 1;
    else
        newPos->eof = 0;
}

void SetFilePosition(struct FileHandle * fileHandle, const struct FilePosition * newPos)
{
    int ret;

    if (newPos->eof)
        ret = fseek((FILE *)fileHandle, 0, SEEK_END);
    else
        ret = fseek((FILE *)fileHandle, (long)newPos->pos, SEEK_SET);

    if (ret != 0)
        exit(1);
}

int IsEOF(struct FileHandle * fileHandle)
{
    if (feof((FILE *)fileHandle))
        return 1;
    else
        return 0;
}

unsigned long long GetFileSize(struct FileHandle * fileHandle)
{
    fpos_t pos;
    long size;

    if (fgetpos((FILE *)fileHandle, &pos) != 0)
        exit(1);

    if (fseek((FILE *)fileHandle, 0, SEEK_END) != 0)
        exit(1);
    size = ftell((FILE *)fileHandle);

    if (fsetpos((FILE *)fileHandle, &pos) != 0)
        exit(1);

    return (unsigned long long)size;
}

void SeekFileBegin(struct FileHandle * fileHandle)
{
    if (fseek((FILE *)fileHandle, 0, SEEK_SET) != 0)
        exit(1);
}

void SeekFileEnd(struct FileHandle * fileHandle)
{
    if (fseek((FILE *)fileHandle, 0, SEEK_END) != 0)
        exit(1);
}

/*
struct InputStreamHandle * OpenInputStreamFromFile(struct FileHandle * fileHandle)
{

}

struct InputStreamHandle * OpenInputStreamFromStdin()
{

}

void IsEOS(struct InputStreamHandle *)
{

}

struct OutputStreamHandle * OpenOutputStreamFromFile(struct FileHandle * fileHandle)
{

}

struct OutputStreamHandle * OpenOutputStreamFromStdout()
{

}

struct OutputStreamHandle * OpenOutputStreamFromStderr()
{

}

void SetEOS(struct OutputStreamHandle *)
{

}

void PutChar(struct OutputStreamHandle * outputStreamHandle, int c)
{

}

int GetChar(struct InputStreamHandle * inputStreamHandle)
{

}
*/

void MemSet(void * begin, void * end, char value)
{
    if (end > begin)
    {
        memset(begin, value, (char *)end - (char *)begin);
    }
}
int MemCmp(const void * left, const void * right, unsigned long long num)
{
    return memcmp(left, right, num);
}
void MemCpy(const void * from, void * to, unsigned long long num)
{
    memcpy(to, from, num);
}
void MemMov(const void * from, void * to, unsigned long long num)
{
    memmove(to, from, num);
}

unsigned long long CStrLen(const char * str)
{
    return strlen(str);
}
int CStrCmp(const char * s1, const char * s2)
{
    return strcmp(s1, s2);
}
void CStrCpy(const char * from, char * to)
{
    strcpy_s(to, strlen(from) + 1, from);
}

float FltCeil(float f)
{
    return ceilf(f);
}
float FltFloor(float f)
{
    return floorf(f);
}
float FltMod(float numer, float denom)
{
    return fmodf(numer, denom);
}
float FltAbs(float f)
{
    return fabsf(f);
}

float FltSin(float x)
{
    return sinf(x);
}
float FltCos(float x)
{
    return cosf(x);
}
float FltTan(float x)
{
    return tanf(x);
}

float FltLog(float x)
{
    return logf(x);
}
float FltPow(float base, float exponent)
{
    return powf(base, exponent);
}
float FltSqrt(float x)
{
    return sqrtf(x);
}

void StdScanf(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vscanf(fmt, args);
    va_end(args);
}
void StdPrintf(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void * Alloc(unsigned long int size)
{
    void * p;
    p = malloc(size);
    printf("malloc 0x%016p, size = %lu\n", p, size);
    return p;
}

int Free(void * ptr)
{
    printf("free   0x%016p\n", ptr);
    free(ptr);
    return 0;
}

long GetClock()
{
    return clock();
}

long long GetCurrentTime()
{
    time_t now;
    time(&now);
    return (long long)now;
}

void ProcAbort()
{
    abort();
}
void ProcExit(int status)
{
    exit(status);
}
int ProcAtExit(void(*func)())
{
    atexit((void(*)(void))func);
    return 0;
}
