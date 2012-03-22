#ifndef _SMV_H
#define _SMV_H

typedef struct ModList
{
    struct Module  *this;
    struct ModList *next;
} ModList;

typedef struct Module
{
    char *name;
    int  usglob;         /* = 1 if using variable of main */
    int  runable;        /* if runable, start value of pc should be given by parameter */
    int  type;           /* 'M'---main , 'P'----active, 'C' ---channel */
    int  pcmax;         /* max of pc in this module */
    struct Express   *prov;     /* provide clause */
    struct VarList   *varl;     /* local variable list */
//    struct VarList   *instancel;
    struct Varinf    *varpc;
    struct NextList  *nextl;    /* next part */
    struct LabNode   *pctran;   /* pc trans part */
    struct PointOfScheduleChange *atomicPoint;      /* for exclusive run of module */
    struct PointOfScheduleChange *blockPoint;
} Module;

typedef struct Varinf           /* every element of array is a independent var */
{
    char *name;
    int  ini;                 /* init value */
    struct Express  *iexpress;       /* init express when run*/
    int  para;                /* para = 1 if parameter */
    int  type;                /* SIGNED, BOOLEAN, ENUM, INTEGER, PROCESS */
    unsigned char  isarray;
//  unsigned char  isenum;
//  unsigned char  isrange;
    struct Module  *chmod;      /* if type is PROCESS, pname will be the MOUDLE */
    struct Module  *context;
    int    bits;                /* the bits of unsigned/signed type */
    int    Imin;                /* min index of array */
    int    Imax;                /* max index of array */
    int    hasnext;             /* if this variable writted */
//  int    index;               /* index of array element */
//  struct Varinf  *nxt;        /* for local variable list */
} Varinf;

typedef struct VarList              /* variable decl list */
{
    struct Varinf  *this;
    struct VarList *nxt;
} VarList;

typedef struct Next
{
    int nowpc;
    int idx;                 /* = '#' if left variable isnot element of an array*/
    struct Varinf  *lvar;
    struct Varinf  *svar;      /* reference variable.e.g sup.p.point */
    struct Express *rval;
    struct Express *cond;      /* when there is conditions */
    struct Next    *nxt;
} Next; 

typedef struct NextList
{
    struct Next     *th;
    struct NextList *nxt;
} NextList;

typedef struct Express
{
   int oper;                     /* operator */
   int priority;
   int type;                     /* SIGNED BOOLEAN INTEGER ENUM */
   int nbit;
   int value;                    /* when const */
   struct Varinf  *var;              /* when variable */
   struct Varinf  *svar;
   struct Express *lexp;
   struct Express *rexp;
} Express;

typedef struct LabNode
{
    int lab;
    struct TranArc *frtarc;
    struct LabNode *next;
} LabNode;

typedef struct TranArc
{
    int adjvex;
    struct TranArc *nxtarc;
    struct Express *cond;
} TranArc;

typedef struct PointOfScheduleChange
{
    int mark;
    struct Express *condition;
    struct PointOfScheduleChange *next;
} PointOfScheduleChange;

#ifndef UNSIGNED
#define UNSIGNED 5
#endif

#define BOOLEAN  1      // 000001
#define ZEROORONE 2     // 000010 --0, 1 not sure it's boolean or integer
#define INTEGER  4      // 000100
#define SIGNED   8      // 001000
#define ENUM     16     // 010000
#define PROCESS  32     // 100000

#define XOR  20
#define NEXT 30

#if 0
#define EXP  21
#define SEXP 22
#define FEND 23
#define CHCOND 24
#define JUMP 25
#define END  26
#define SEQ  27
#define TOATOM 28
#endif

#endif

