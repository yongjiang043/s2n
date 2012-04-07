/***** spin: spinlex.c *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

#include <stdlib.h>
#include <ctype.h>
#include "main.h"
#include "func.h"
#include "spin.h"
#include "parse.h"
#include "y.tab.h"

char    yytext[2048];
int     lineno = 1;
static unsigned char    in_comment = 0;
static int    checkName(char *);

#if 1
#define token(y)          { if (in_comment) goto again; \
                            yylval = nn(ZN,0,ZN,ZN); return y; }

#define valToken(x, y)    { if (in_comment) goto again; \
                            yylval = nn(ZN,0,ZN,ZN); yylval->val = x; return y; }

#define symToken(x, y)    { if (in_comment) goto again; \
                            yylval = nn(ZN,0,ZN,ZN); yylval->sym = x; return y; }
#else
#define token(y)          { yylval = nn(ZN,0,ZN,ZN); \
                            if (!in_comment) return y; else goto again; }

#define valToken(x, y)    { yylval = nn(ZN,0,ZN,ZN); yylval->val = x; \
                            if (!in_comment) return y; else goto again; }

#define symToken(x, y)    { yylval = nn(ZN,0,ZN,ZN); yylval->sym = x; \
                            if (!in_comment) return y; else goto again; }
#endif

#if 1
#define getChar()     getc(yyin)
#define unGetch(c)    ungetc(c,yyin)

#else

static int getChar()
{
    int c;
    c = getc(yyin);
#if 0
    if (Inlining < 0)
    {
        c = getc(yyin);
    }
    else
    {
        c = getinline();
    }
    if (0)
    {
        printf("<%c:%d>[%d] ", c, c, Inlining);
    }
#endif
    return c;
}

static void unGetch(int c)
{
    ungetc(c,yyin);
#if 0
    if (Inlining<0)
    {
        ungetc(c,yyin);
    }
    else
    {
        uninline();
    }
    if (0)
    {
        printf("<bs>");
    }
#endif
}
#endif

static int notQuote(int c)
{
    return (c != '\"' && c != '\n');
}

static int isAlnum(int c)
{
    return (isalnum(c) || c == '_');
}

static int isAlpha(int c)
{
    return isalpha(c);    /* could be macro */
}
       
static int isDigit(int c)
{
    return isdigit(c);    /* could be macro */
}

static void getWord(int first, int (*tst)(int))
{
    int i=0, c;

    yytext[i++]= (char) first;
    while (tst(c = getChar()))
    {
        yytext[i++] = (char) c;
        if (c == '\\')
        {
            c = getChar();
            yytext[i++] = (char) c;    /* no tst */
        }
    }
    yytext[i] = '\0';
    unGetch(c);
}

static int follow(int tok, int ifyes, int ifno)
{
    int c;

    if ((c = getChar()) == tok)
    {
        return ifyes;
    }
    unGetch(c);

    return ifno;
}

static void doDirective(int first)
{
    int c = first;    /* handles lines starting with pound */

    getWord(c, isAlpha);

    if (strcmp(yytext, "#ident") == 0)
    {
        goto done;
    }

    if ((c = getChar()) != ' ')
    {
        info(ERROR, "malformed preprocessor directive - # .");
    }

    if (!isDigit(c = getChar()))
    {
        info(ERROR, "malformed preprocessor directive - # .lineno");
    }

    getWord(c, isDigit);
    lineno = atoi(yytext);    /* pickup the line number */

    if ((c = getChar()) == '\n')
    {
        return;    /* no filename */
    }

    if (c != ' ')
    {
        info(ERROR, "malformed preprocessor directive - .fname");
    }

    if ((c = getChar()) != '\"')
    {
        info(ERROR, "malformed preprocessor directive - .fname");
    }

    getWord(c, notQuote);
    if (getChar() != '\"')
    {
        info(ERROR, "malformed preprocessor directive - fname.");
    }

    strcat(yytext, "\"");

done:
    while (getChar() != '\n') ;
}

static int lex()
{
    int c;

again:
    c = getChar();
    yytext[0] = (char) c;
    yytext[1] = '\0';
    switch (c)
    {
        case EOF:
        {
            return c;
        }
        case '\n':        /* newline */
        {
            lineno++;
        }
        case '\r':        /* carriage return */
        {
            goto again;
        }

        case  ' ': case '\t': case '\f':    /* white space */
        {
            goto again;
        }

        case '#':        /* preprocessor directive */
        {
            if (in_comment)
            {
                goto again;
            }
        }
        doDirective(c);
        goto again;

        case '\"':
        {
            getWord(c, notQuote);
            if (getChar() != '\"')
            {
                info(ERROR, "string %s not terminated", yytext);
            }
            strcat(yytext, "\"");
            symToken(lookup(yytext), STRING);
        }
        case '\'':    /* new 3.0.9 */
        {
            c = getChar();
            if (c == '\\')
            {
                c = getChar();
                if (c == 'n')
                {
                    c = '\n';
                }
                else if (c == 'r')
                {
                    c = '\r';
                }
                else if (c == 't')
                {
                    c = '\t';
                }
                else if (c == 'f')
                {
                    c = '\f';
                }
            }
            if (getChar() != '\'' && !in_comment)
            {
                info(ERROR, "character quote missing: %s", yytext);
            }
            valToken(c, CONST);
        }

        default:
        {
            break;
        }
    }

    if (isDigit(c))
    {
        getWord(c, isDigit);
        valToken(atoi(yytext), CONST);
    }

    if (isAlpha(c) || c == '_')
    {
        getWord(c, isAlnum);
        if (!in_comment)
        {
            c = checkName(yytext);
            if (c)
            {
                return c;
            }
            /* else fall through */
        }
        goto again;
    }

    switch (c)
    {
        case '/':
        {
            c = follow('*', 0, '/');
            if (!c)
            {
                in_comment = 1; goto again;
            }
            break;
        }
        case '*':
        {
            c = follow('/', 0, '*');
            if (!c)
            {
                in_comment = 0; goto again;
            }
            break;
        }
        case ':':
        {
            c = follow(':', SEP, ':'); break;
        }
        case '-':
        {
            c = follow('>', SEMI, follow('-', DECR, '-')); break;
        }
        case '+':
        {
            c = follow('+', INCR, '+'); break;
        }
        case '<':
        {
            c = follow('<', LSHIFT, follow('=', LE, LT)); break;
        }
        case '>':
        {
            c = follow('>', RSHIFT, follow('=', GE, GT)); break;
        }
        case '=':
        {
            c = follow('=', EQ, ASGN); break;
        }
        case '!':
        {
            c = follow('=', NE, follow('!', O_SND, SND)); break;
        }
        case '?':
        {
            c = follow('?', R_RCV, RCV); break;
        }
        case '&':
        {
            c = follow('&', AND, '&'); break;
        }
        case '|':
        {
            c = follow('|', OR, '|'); break;
        }
        case ';':
        {
            c = SEMI; break;
        }
        default :
        {
            break;
        }
    }
    token(c);
}

static struct
{
    char *s;    int tok;    int val;    char *sym;
} Names[] = {
    {"active",    ACTIVE,        0,        0},
    {"assert",    ASSERT,        0,        0},
    {"atomic",    ATOMIC,        0,        0},
    {"bit",       TYPE,        BIT,        0},
    {"bool",      TYPE,        BIT,        0},
    {"break",     BREAK,         0,        0},
    {"byte",      TYPE,       BYTE,        0},
    {"c_code",    C_CODE,        0,        0},
    {"c_decl",    C_DECL,        0,        0},
    {"c_expr",    C_EXPR,        0,        0},
    {"c_state",   C_STATE,       0,        0},
    {"c_track",   C_TRACK,       0,        0},
    {"D_proctype",D_PROCTYPE,    0,        0},
    {"do",        DO,            0,        0},
    {"chan",      TYPE,       CHAN,        0},
    {"else",      ELSE,          0,        0},
    {"empty",     EMPTY,         0,        0},
    {"enabled",   ENABLED,       0,        0},
    {"eval",      EVAL,          0,        0},
    {"false",     CONST,         0,        0},
    {"fi",        FI,            0,        0},
    {"full",      FULL,          0,        0},
    {"goto",      GOTO,          0,        0},
    {"hidden",    HIDDEN,        0,        ":hide:"},
    {"if",        IF,            0,        0},
    {"init",      INIT,          0,        "initial"},
    {"int",       TYPE,        INT,        0},
    {"len",       LEN,           0,        0},
    {"local",     ISLOCAL,       0,        ":local:"},
    {"mtype",     TYPE,      MTYPE,        0},
    {"nempty",    NEMPTY,        0,        0},
    {"never",     CLAIM,         0,        ":never:"},
    {"nfull",     NFULL,         0,        0},
    {"notrace",   TRACE,         0,        ":notrace:"},
    {"np_",       NONPROGRESS,   0,        0},
    {"od",        OD,            0,        0},
    {"of",        OF,            0,        0},
    {"pc_value",  PC_VAL,        0,        0},
    {"pid",       TYPE,       BYTE,        0},
    {"printf",    PRINT,         0,        0},
    {"printm",    PRINTM,        0,        0},
    {"priority",  PRIORITY,      0,        0},
    {"proctype",  PROCTYPE,      0,        0},
    {"provided",  PROVIDED,      0,        0},
    {"run",       RUN,           0,        0},
    {"d_step",    D_STEP,        0,        0},
    {"inline",    INLINE,        0,        0},
    {"short",     TYPE,      SHORT,        0},
    {"skip",      CONST,         1,        0},
    {"timeout",   TIMEOUT,       0,        0},
    {"trace",     TRACE,         0,        ":trace:"},
    {"true",      CONST,         1,        0},
    {"show",      SHOW,          0,        ":show:"},
    {"typedef",   TYPEDEF,       0,        0},
    {"unless",    UNLESS,        0,        0},
    {"unsigned",  TYPE,   UNSIGNED,        0},
    {"xr",        XU,           XR,        0},
    {"xs",        XU,           XS,        0},
    {0,           0,             0,        0},
};

static int checkName(char *s)
{
    int i;

    yylval = nn(ZN, 0, ZN, ZN);
    for (i = 0; Names[i].s; i++)            /*keyword*/
    {
        if (strcmp(s, Names[i].s) == 0)
        {
            yylval->val = Names[i].val;
            if (Names[i].sym)
            {
                yylval->sym = lookup(Names[i].sym);
            }
            return Names[i].tok;
        }
    }

    if ((yylval->val = isMtype(s)) != 0)        /*mtype*/
    {
        yylval->ismtyp = 1;
        return CONST;
    }
    yylval->sym = lookup(s);    /* symbol table */
/*  if (isutype(s))
    {
        return UNAME;
    }*/
    if (isProctype(s))
    {
        return PNAME;
    }
/*  if (iseqname(s))
    {
        return INAME;
    }*/

    return NAME;
}

int yylex(void)
{
    static int last = 0;
    static int hold = 0;
    int c;

    /*
     * repair two common syntax mistakes with
     * semi-colons before or after a '}'
     */
    if (hold)
    {
        c = hold;
        hold = 0;
    }
    else
    {
        c = lex();
        if (last == ELSE &&  c != SEMI &&  c != FI)
        {
            hold = c;
            last = 0;
            return SEMI;
        }
        if (last == '}'
        &&  c != PROCTYPE
        &&  c != INIT
        &&  c != CLAIM
        &&  c != SEP
        &&  c != FI
        &&  c != OD
        &&  c != '}'
        &&  c != UNLESS
        &&  c != SEMI
        &&  c != EOF)
        {
            hold = c;
            last = 0;
            return SEMI;    /* insert ';' */
        }
        if (c == SEMI)
        {    /* if context, we're not in a typedef
             * because they're global.
             * if owner, we're at the end of a ref
             * to a struct field -- prevent that the
             * lookahead is interpreted as a field of
             * the same struct...
             */
            hold = lex();    /* look ahead */
            if (hold == '}' ||  hold == SEMI)
            {
                c = hold; /* omit ';' */
                hold = 0;
            }
        }
    }
    last = c;

    return c;
}
