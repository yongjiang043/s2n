/***** main : main.h *****/
/* interface of main.c, some global variables and functions. */

#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>

extern FILE *yyin;
extern char out1[];
extern int  debug;
extern int  ifmark;
extern char *srcfile;
extern char *trgfile;
extern double costTime;

void explain(int);
void usetip();

#endif
