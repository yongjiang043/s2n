#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "spin.h"
#include "main.h"
#include "spinlex.h"

FILE *logfile;

char *emalloc(size_t);
void allDone(int);

static char *set_color(const char *format, int color)
{
    char *string;

    string = (char *)emalloc(strlen(format)+20);
    sprintf(string, "\033[;%dm", color);
    strcat(string, format);
    strcat(string, "\033[0m");

    return string;
}

static int message(int color, char *attach, const char *format, va_list ptr)
{
    int n = 0;
    char *string, *msg;

    msg = (char *)emalloc(strlen(format) + 10);
    strcpy(msg, attach);
    strcat(msg, format);
#ifdef LINUX
    string = set_color(msg, color);
#else
    string = msg;
#endif

    n = vfprintf(logfile, string, ptr);
    fprintf(logfile, "\n");

    return n;
}

int info(Log log, const char *format, ...)
{
    int n = 0;
    va_list arg_ptr;

    va_start(arg_ptr, format);

    switch (log)
    {
        case NOTE:
        {
            n = message(32, "", format, arg_ptr);
            break;
        }
        case DEBUG:
        {
            if (debug)
            {
                n = message(34, "", format, arg_ptr);
            }
            break;
        }
        case WARN:
        {
            n = message(33, "warn: ", format, arg_ptr);
            break;
        }
        case ERROR:
        {
            n = message(31, "error: ", format, arg_ptr);
            allDone(1);
            break;
        }
    }

    va_end(arg_ptr);

    return n;
}

void allDone(int estatus)
{
    if(strlen(out1) > 0)
    {
        (void) unlink((const char *)out1);
    }

    if (!estatus)
    {
        info(NOTE, "all succeeded! Using time %lf seconds", costTime);
    }
    exit(estatus);
}

char *sstrcpy(char *dest, const char *src, int begin, int end)
{
    char *destcpy = dest;
    int i, leng;

    if (dest == NULL || src == NULL)
    {
        printf("warn: arguments of sstrcpy cannot be NULL! Please check your argument.\n");
        return NULL;
    }

    leng = strlen(src);
    if (0 <= begin && end < leng && begin <= end)
    {
        for (i = begin; i<= end; i++)
        {
            *dest++ = src[i];
        }
    }
    else
    {
        printf("warn: your arguments of sstrcpy is invalid!\n");
        return NULL;
    }
    *dest = '\0';

    return destcpy;
}

char *emalloc(size_t n)
{
    char *tmp;

    if (n == 0)
    {
        return NULL;
    }

    if (!(tmp = (char *)malloc(n)))
    {
        info(ERROR, "not enough memory");
    }

    memset(tmp, 0, n);

    return tmp;
}

void 
nspace(int n, FILE *f)
{
    int i;

    for(i=0; i<n; i++)
    {
        fputs(" ", f);
    }
}

/*
 * return 2^n
 */
int power(int n)
{
    int result = 1;

    result = result << n;
//    printf("2^%d is %d\n", n, result);

    return result;
}
