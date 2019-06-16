[assert]

macro: assert


[char]

int isalnum(int c);
int isalpha(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);


[int]

int         abs(int n);
long int    labs(long int n);
div_t       div(int numer, int denom);
ldiv_t      ldiv(long int numer, long int denom);

macro: limits, bits in char
typedef: ...


[float]

macro: format, constant, flags
fenv control


[math]

double cos(double x);
double sin(double x);
double tan(double x);
double acos(double x);
double asin(double x);
double atan(double x);
double atan2(double y, double x);

double cosh(double x);
double sinh(double x);
double tanh(double x);

double exp(double x);
double frexp(double x, int * exp);
double ldexp(double x, int exp);
double log(double x);
double log10(double x);
double modf(double x, double * intpart);

double pow(double base, double exponent);
double sqrt(double x);

double ceil(double x);
double floor(double x);
double fmod(double numer, double denom);

double fabs(double x);


[string]

void * memcpy(void * destination, const void * source, size_t num);
void * memmove(void * destination, const void * source, size_t num);
char * strcpy(char * destination, const char * source);
char * strncpy(char * destination, const char * source, size_t num);

char * strcat(char * destination, const char * source);
char * strncat(char * destination, const char * source, size_t num);

int memcmp(const void * ptr1, const void * ptr2, size_t num);
int strcmp(const char * str1, const char * str2);
int strcoll(const char * str1, const char * str2);
int strncmp(const char * str1, const char * str2, size_t num);
size_t strxfrm(char * destination, const char * source, size_t num);

void * memchr(const void * ptr, int value, size_t num);
char * strchr(const char * str, int character);
size_t strcspn(const char * str1, const char * str2);
char * strpbrk(const char * str1, const char * str2);
char * strrchr(const char * str, int character);
size_t strspn(const char * str1, const char * str2);
char * strstr(const char * str1, const char * str2);
char * strtok(char * str, const char * delimiters);

void * memset(void * ptr, int value, size_t num);
size_t strlen(const char * str);


[serialization]

double atof(const char * str);
int atoi(const char * str);
long int atol(const char * str);
double strtod(const char * str, char ** endptr);
long int strtol(const char * str, char ** endptr, int base);
unsigned long int strtoul(const char * str, char ** endptr, int base);

to string: see io formatting


[encoding]

int mblen(const char * pmb, size_t max);

int mbtowc(wchar_t * pwc, const char * pmb, size_t max);
int wctomb(char * pmb, wchar_t wc);

size_t mbrtoc16(char16_t * pc16, const char * pmb, size_t max, mbstate_t * ps);
size_t c16rtomb(char * pmb, char16_t c16, mbstate_t * ps);

size_t mbrtoc32(char32_t * pc32, const char * pmb, size_t max, mbstate_t * ps);
size_t c32rtomb(char * pmb, char32_t c32, mbstate_t * ps);


[memory management]

void *  malloc(unsigned long int size);
void    free(void * ptr);
void * calloc(size_t num, size_t size);
void * realloc(void * ptr, size_t size);


[time]

clock_t clock(void);
double difftime(time_t end, time_t beginning);
time_t mktime(struct tm * timeptr);
time_t time(time_t * timer);

char * asctime(const struct tm * timeptr);
char * ctime(const time_t * timer);
struct tm * gmtime(const time_t * timer);
struct tm * localtime(const time_t * timer);
size_t strftime(char * ptr, size_t maxsize, const char * format, const struct tm * timeptr);


[io]

int     remove(const char * filename);
int     rename(const char * oldname, const char * newname);

FILE *  fopen(const char * filename, const char * mode);
FILE *  freopen(const char * filename, const char * mode, FILE * stream);
int     fflush(FILE * stream);
int     fclose(FILE * stream);

int     fgetpos(FILE * stream, fpos_t * pos);
int     fsetpos(FILE * stream, const fpos_t * pos);
int     fseek(FILE * stream, long int offset, int origin);
long int ftell(FILE * stream);
void    rewind(FILE * stream);
int     feof(FILE * stream);

void    setbuf(FILE * stream, char * buffer);
int     setvbuf(FILE * stream, char * buffer, int mode, size_t size);

size_t  fread(void * ptr, size_t size, size_t count, FILE * stream);
int     fgetc(FILE * stream);
char *  fgets(char * str, int num, FILE * stream);

size_t  fwrite(const void * ptr, size_t size, size_t count, FILE * stream);
int     fputc(int character, FILE * stream);
int     fputs(const char * str, FILE * stream);

int     getchar(void);
char *  gets(char * str);

int     putchar(int character);
int     puts(const char * str);

int     ungetc(int character, FILE * stream);

int     fscanf(FILE * stream, const char * format, ...);
int     sscanf(const char * s, const char * format, ...);
int     scanf(const char * format, ...);

int     fprintf(FILE * stream, const char * format, ...);
int     sprintf(char * str, const char * format, ...);
int     printf(const char * format, ...);

int     vfprintf(FILE * stream, const char * format, va_list arg);
int     vsprintf(char * s, const char * format, va_list arg);
int     vprintf(const char * format, va_list arg);

void    clearerr(FILE * stream);
int     ferror(FILE * stream);
void    perror(const char * str);


[env]

void    abort();
void    exit(int status);

