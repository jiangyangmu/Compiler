/* ctype.h */
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

/* errno.h */
extern int errno;

/* math.h */
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
double frexp(double x, int* exp);
double ldexp(double x, int exp);
double log(double x);
double log10(double x);
double modf(double x, double* intpart);

double pow(double base, double exponent);
double sqrt(double x);

double ceil(double x);
double floor(double x);
double fmod(double numer, double denom);

double fabs(double x);

/* stdio.h */
int putchar(int c);

int main()
{
    int c;

    return 0;
}
