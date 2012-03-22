/******spin: spin.h ************/
/* struct for spin AST */
#ifndef _SPIN_H
#define _SPIN_H

#include <stdio.h>
#include <string.h>
#include "smv.h"

typedef struct Lextok
{
    unsigned short ntyp;    /* node type */
    short ismtyp;           /* CONST derived from MTYP */
    int val;                /* value attribute */
    int ln;                 /* line number */
    int lab;                /* marking */
    int inatom;             /* part of atomic sequence */
    int rev;                /* rendezvous */
    int indstep;            /* part of d_step sequence */
//  struct Symbol   *fn;        /* file name */
    struct Symbol   *sym;       /* symbol reference */
    struct Sequence *sq;        /* sequence */
    struct SeqList  *sl;        /* sequence list */
    struct Express  *excut_cond;
    struct Express  *forecond;
    struct Lextok   *lft, *rgt; /* children in parse tree */
} Lextok;

typedef struct Symbol
{
    char *name;
//  int  Nid;                  /* unique number for the name */
    unsigned short type;       /* bit,short,.., chan,struct  */
//  unsigned char hidden;
                               /* bit-flags:
                                  1=hide, 2=show,
                                  4=bit-equiv,   8=byte-equiv,
                                 16=formal par, 32=inline par,
                                 64=treat as if local; 128=read at least once
                               */
    int rev;
    unsigned char isarray;     /* set if decl specifies array bound */
    int nbits;                 /* optional width specifier */
    int nel;                   /* 1 if scalar, >1 if array   */
//  int setat;                 /* last depth value changed   */
//  int *val;                  /* runtime value(s), initl 0  */
//  Lextok **Sval;             /* values for structures */

//  int xu;                    /* exclusive r or w by 1 pid  */
//  struct Symbol *xup[2];     /* xr or xs proctype  */
//  struct Access *access;     /* e.g., senders and receives of chan */
    Lextok *ini;               /* initial value, or chan-def */
    struct Symbol *context;    /* 0 if global, or procname */
    struct Symbol *next;       /* linked list */
} Symbol;

/*
typedef struct Ordered         // links all names in Symbol table
{
    struct Symbol  *entry;
    struct Ordered *next;
} Ordered;
*/

typedef struct Element
{
    Lextok *n;        /* defines the type & contents */
    int eleno;        /* identifies this el within system */
//  int seqno;        /* identifies this el within a proc */
//  int merge;        /* set by -O if step can be merged */
//  int merge_start;
//  int merge_single;
//  short merge_in;      /* nr of incoming edges */
//  short merge_mark;    /* state was generated in merge sequence */
//  unsigned int status;     /* used by analyzer generator  */
//  struct FSM_use *dead;    /* optional dead variable list */
    struct SeqList *sub;     /* subsequences, for compounds */
    struct SeqList *esc;     /* zero or more escape sequences */
//  struct Element *Nxt;     /* linked list - for global lookup */
    struct Element *nxt;     /* linked list - program structure */
} Element;

typedef struct Sequence
{
    Element *frst;
    Element *last;        /* links onto continuations */
    Element *extent;      /* last element in original */
//  int maxel;            /* 1+largest id in sequence */
} Sequence;

typedef struct SeqList
{
    Sequence *this;       /* one sequence */
    struct SeqList *nxt;  /* linked list  */
} SeqList;

typedef struct LexList
{
    Lextok *this;    /* global variable definition*/
    Module *cmod;    /* if chanlist */
    struct LexList *next;
} LexList;

typedef struct Label
{
    Symbol *s;        /*label symbol*/
    Symbol *c;        /*context*/
    Element *e;       /*target*/
    struct Label *nxt;
} Label;

typedef struct Lbreak         /*for break stack in loop*/
{
    Symbol *l;        /*name like ":b%d"*/
    struct Lbreak *nxt;
} Lbreak;

typedef struct ProcList
{
    Symbol   *n;                /* name                      */
    Lextok   *p;                /* parameters                */
    Sequence *s;                /* body                      */
    Lextok   *prov;             /* provided clause           */
    int runable;
    int pronum;                 /* number of instantiation   */
    int max_mark;               /* max of mark               */
//  short tn;                   /* ordinal number            */
//  unsigned char det;          /* deterministic             */
//  unsigned char unsafe;       /* contains global var inits */
    struct ProcList *nxt;       /* linked list               */
    struct Module *m;
} ProcList;

/*   DynamicList RunList OneRun are for map of dynamic runs  */

typedef struct DynamicList
{
    int    indegree;
    struct ProcList *host;
    struct RunList *rl;
    struct DynamicList *nxt;
    struct DynamicList *nxtTopoEle;
    struct DynamicList *nxtStackEle;
} DynamicList;

typedef struct RunList
{
    char *guest;
    struct OneRun *run;
    struct RunList *nxt;
} RunList;

typedef struct OneRun
{
    Lextok *r;
    struct OneRun *nxt;
} OneRun;

/*
typedef struct Multiseq
{
    int opt;
    unsigned short fo;
    struct Express *cond;
    struct Element *now;
    struct Multiseq *next;
} Multiseq;
*/

typedef enum
{
    NOTE,
    DEBUG,
    ERROR,
    WARN
} Log;

typedef Lextok *Lexptr;
#define YYSTYPE Lexptr

#define ZN  (Lextok *)0
#define ZS  (Symbol *)0
#define ZE  (Element *)0

#define Nhash    55         /* slots in symbol hash-table */

#define XS        2         /* non-shared send-only    */
#define XR        1         /* non-shared receive-only */

#define UNSIGNED  5         /* val defines width in bits */
#define BIT       1         /* also equal to width in bits */
#define BYTE      8         /* ditto */
#define SHORT     16        /* ditto */
#define INT       32        /* ditto */
#define CHAN      64        /* not */
#define STRUCT    128       /* user defined structure name */

#endif
