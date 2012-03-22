/***** func : func.h *****/
/* some functions for system or handle exception*/

#ifndef _FUNC_H
#define _FUNC_H

#include "spin.h"

extern FILE *logfile;

int  info(Log, const char *, ...);
void allDone(int);
char *sstrcpy(char *, const char *, int, int);
char *emalloc(size_t);
void nspace(int, FILE *);
int power(int);

#endif
