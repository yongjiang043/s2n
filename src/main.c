#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "spin.h"
#include "func.h"
#include "parse.h"
#include "global.h"
#include "y.tab.h"

#define CPP "cpp"

FILE  *yyin, *yyout;
double costTime;
char  out1[8];                 // name of the file after preprocessed 
int   debug = 0;               // if debug is needed 
int   ifmark = 0;              // whether users need the file labeling.pml after labeled 
char  *srcfile = NULL;         // name of input file 
char  *trgfile = NULL;         // name of target file 

static char *PreProc = CPP;
static char Operator[] = "operator: ";
static char Keyword[] = "keyword: ";
static char Function[] = "function-name: ";

void usetip()
{
    printf("s2n: %s\n", S2NVersion);
    printf("usage: s2n [-option]* source_file [-o target_file]\n");
    printf("\tNote: file must always be the last argument\n");
    printf("\t-h/H help tip.\n");
    printf("\t-m/M generates spin file with mark -- source_file_mark.pml\n");
    printf("\t-d/D show debug information.\n");
}

void explain(int n)
{
    FILE *fd = stdout;
    switch (n)
    {
        default:
        {
            if (n > 0 && n < 256)
            {
                fprintf(fd, "'%c' = ", n);
            }
            fprintf(fd, "%d", n);
            break;
        }
        case '\b':
        {
            fprintf(fd, "\\b"); break;
        }
        case '\t':
        {
            fprintf(fd, "\\t"); break;
        }
        case '\f':
        {
            fprintf(fd, "\\f"); break;
        }
        case '\n':
        {
            fprintf(fd, "\\n"); break;
        }
        case '\r':
        {
            fprintf(fd, "\\r"); break;
        }
        case 'c':
        {
            fprintf(fd, "condition"); break;
        }
        case 's':
        {
            fprintf(fd, "send"); break;
        }
        case 'r':
        {
            fprintf(fd, "recv"); break;
        }
        case 'R':
        {
            fprintf(fd, "recv poll %s", Operator); break;
        }
        case '@':
        {
            fprintf(fd, "@"); break;
        }
        case '?':
        {
            fprintf(fd, "(x->y:z)"); break;
        }
        case ACTIVE:
        {
            fprintf(fd, "%sactive", Keyword); break;
        }
        case AND:
        {
            fprintf(fd, "%s&&", Operator); break;
        }
        case ASGN:
        {
            fprintf(fd, "%s=", Operator); break;
        }
        case ASSERT:
        {
            fprintf(fd, "%sassert", Function); break;
        }
        case ATOMIC:
        {
            fprintf(fd, "%satomic", Keyword); break;
        }
        case BREAK:
        {
            fprintf(fd, "%sbreak", Keyword); break;
        }
        case C_CODE:
        {
            fprintf(fd, "%sc_code", Keyword); break;
        }
        case C_DECL:
        {
            fprintf(fd, "%sc_decl", Keyword); break;
        }
        case C_EXPR:
        {
            fprintf(fd, "%sc_expr", Keyword); break;
        }
        case C_STATE:
        {
            fprintf(fd, "%sc_state",Keyword); break;
        }
        case C_TRACK:
        {
            fprintf(fd, "%sc_track", Keyword); break;
        }
        case CLAIM:
        {
            fprintf(fd, "%snever", Keyword); break;
        }
        case CONST:
        {
            fprintf(fd, "a constant"); break;
        }
        case DECR:
        {
            fprintf(fd, "%s--", Operator); break;
        }
        case D_STEP:
        {
            fprintf(fd, "%sd_step", Keyword); break;
        }
        case D_PROCTYPE:
        {
            fprintf(fd, "%sd_proctype", Keyword); break;
        }
        case DO:
        {
            fprintf(fd, "%sdo", Keyword); break;
        }
        case DOT:
        {
            fprintf(fd, "."); break;
        }
        case ELSE:
        {
            fprintf(fd, "%selse", Keyword); break;
        }
        case EMPTY:
        {
            fprintf(fd, "%sempty", Function); break;
        }
        case ENABLED:
        {
            fprintf(fd, "%senabled", Function); break;
        }
        case EQ:
        {
            fprintf(fd, "%s==", Operator); break;
        }
        case EVAL:
        {
            fprintf(fd, "%seval", Function); break;
        }
        case FI:
        {
            fprintf(fd, "%sfi", Keyword); break;
        }
        case FULL:
        {
            fprintf(fd, "%sfull", Function); break;
        }
        case GE:
        {
            fprintf(fd, "%s>=", Operator); break;
        }
        case GOTO:
        {
            fprintf(fd, "%sgoto", Keyword); break;
        }
        case GT:
        {
            fprintf(fd, "%s>", Operator); break;
        }
        case HIDDEN:
        {
            fprintf(fd, "%shidden", Keyword); break;
        }
        case IF:
        {
            fprintf(fd, "%sif", Keyword); break;
        }
        case INCR:
        {
            fprintf(fd, "%s++", Operator); break;
        }
        case INAME:
        {
            fprintf(fd, "inline name"); break;
        }
        case INLINE:
        {
            fprintf(fd, "%sinline", Keyword); break;
        }
        case INIT:
        {
            fprintf(fd, "%sinit", Keyword); break;
        }
        case ISLOCAL:
        {
            fprintf(fd, "%slocal", Keyword); break;
        }
        case LABEL:
        {
            fprintf(fd, "a label-name"); break;
        }
        case LE:
        {
            fprintf(fd, "%s<=", Operator); break;
        }
        case LEN:
        {
            fprintf(fd, "%slen", Function); break;
        }
        case LSHIFT:
        {
            fprintf(fd, "%s<<", Operator); break;
        }
        case LT:
        {
            fprintf(fd, "%s<", Operator); break;
        }
        case MTYPE:
        {
            fprintf(fd, "%smtype", Keyword); break;
        }
        case NAME:
        {
            fprintf(fd, "an identifier"); break;
        }
        case NE:
        {
            fprintf(fd, "%s!=", Operator); break;
        }
        case NEG:
        {
            fprintf(fd, "%s! (not)",Operator); break;
        }
        case NEMPTY:
        {
            fprintf(fd, "%snempty", Function); break;
        }
        case NFULL:
        {
            fprintf(fd, "%snfull",  Function); break;
        }
        case NON_ATOMIC:
        {
            fprintf(fd, "sub-sequence"); break;
        }
        case NONPROGRESS:
        {
            fprintf(fd, "%snp_", Function); break;
        }
        case OD:
        {
            fprintf(fd, "%sod", Keyword); break;
        }
        case OF:
        {
            fprintf(fd, "%sof", Keyword); break;
        }
        case OR:
        {
            fprintf(fd, "%s||", Operator); break;
        }
        case O_SND:
        {
            fprintf(fd, "%s!!", Operator); break;
        }
        case PC_VAL:
        {
            fprintf(fd, "%spc_value", Function); break;
        }
        case PNAME:
        {
            fprintf(fd, "process name"); break;
        }
        case PRINT:
        {
            fprintf(fd, "%sprintf", Function); break;
        }
        case PRINTM:
        {
            fprintf(fd, "%sprintm", Function); break;
        }
        case PRIORITY:
        {
            fprintf(fd, "%spriority", Keyword); break;
        }
        case PROCTYPE:
        {
            fprintf(fd, "%sproctype", Keyword); break;
        }
        case PROVIDED:
        {
            fprintf(fd, "%sprovided", Keyword); break;
        }
        case RCV:
        {
            fprintf(fd, "%s?", Operator); break;
        }
        case R_RCV:
        {
            fprintf(fd, "%s??", Operator); break;
        }
        case RSHIFT:
        {
            fprintf(fd, "%s>>", Operator); break;
        }
        case RUN:
        {
            fprintf(fd, "%srun", Operator); break;
        }
        case SEP:
        {
            fprintf(fd, "token: ::"); break;
        }
        case SEMI:
        {
            fprintf(fd, ";"); break;
        }
        case SHOW:
        {
            fprintf(fd, "%sshow", Keyword); break;
        }
        case SND:
        {
            fprintf(fd, "%s!", Operator); break;
        }
        case STRING:
        {
            fprintf(fd, "a string"); break;
        }
        case TRACE:
        {
            fprintf(fd, "%strace", Keyword); break;
        }
        case TIMEOUT:
        {
            fprintf(fd, "%stimeout",Keyword); break;
        }
        case TYPE:
        {
            fprintf(fd, "data typename"); break;
        }
        case TYPEDEF:
        {
            fprintf(fd, "%stypedef", Keyword); break;
        }
        case XU:
        {
            fprintf(fd, "%sx[rs]", Keyword); break;
        }
        case UMIN:
        {
            fprintf(fd, "%s- (unary minus)", Operator); break;
        }
        case UNAME:
        {
            fprintf(fd, "a typename"); break;
        }
        case UNLESS:
        {
            fprintf(fd, "%sunless", Keyword); break;
        }
    }
}

/*
 * Making sure that the input file of s2n should be ended with ".pml".
 */
static void checkSrcname()
{
    int leng = strlen(srcfile);
    char *tail;

    if (leng <= 4) goto error;

    tail = &srcfile[leng - 4];

    if (0 != strcmp(tail, ".pml"))
    {
error:  info(ERROR, "cannot open file %s, spinfile should end with \".pml\"", srcfile);
    }
}

static void preProcess(char *a, char *b)
{
    char precmd[100],cmd[2048] ;

    strcpy(precmd, PreProc);
    if (strlen(precmd) > sizeof(precmd))
    {
        info(ERROR, "the absolute path of preprocessor is too long, aborting");
    }
    sprintf(cmd, "%s %s > %s",precmd, a, b);
    if (system((const char *)cmd))
    {
        (void) unlink((const char *) b);
        info(ERROR, "preprocessing failed");
    }
}

int main(int argc, char *argv[])
{
    clock_t begin = clock(), end;
    yyin = stdout;
    yyout = stdout;
    logfile = stdout;

    while (argc > 1 && argv[1][0] == '-')
    {
        if (strlen(argv[1]) != 2)
        {
            usetip();
            info(ERROR, "your arguments invalid!");
        }

        switch (argv[1][1])
        {
            case 'm': case 'M':
            {
                ifmark = 1; break;    // output the marked file.
            }
            case 'h': case 'H':
            {
                usetip();
                allDone(1);
                break;
            }
            case 'd': case 'D':
            {
                debug = 1; break;     // debug is needed.
            }
            default :
            {
                usetip();
                info(ERROR, "wrong command argument!");
            }
        }
        argc--; argv++;
    }

    if (argc > 1)
    {
        if (argv[1][0] == '\"')
        {
            int i = (int) strlen(argv[1]);
            sstrcpy(srcfile, argv[1], 1, i-2);
        }
        else
        {
            srcfile = (char *)emalloc(strlen(argv[1]) + 1);
            strcpy(srcfile, argv[1]);
        }
        argc--; argv++;

        checkSrcname();

        strcpy(out1,"pre.pml");
        preProcess(srcfile, out1);

        if(!(yyin = fopen(out1,"r")))
        {
            usetip();
            info(ERROR, "cannot open %s\n", out1);
        }

        if (argc > 1)
        {
            if (argc == 3 && !strcmp(argv[1],"-o"))        // set target file
            {
                trgfile = (char *)emalloc(strlen(argv[2]) + 1);
                strcpy(trgfile, argv[2]);
            }
            else
            {
                usetip();
                info(ERROR, "your arguments invalid!");
            }
        }
    }
    else
    {
        usetip();
        info(ERROR, "no input file");
    }

    info(DEBUG, "parse start ...");
    yyparse();
    info(DEBUG, "parse finished.");

    info(DEBUG, "marking start ...");
    marking();
    info(DEBUG, "marking finished.\n");

    info(DEBUG, "translation start ...");
    build();
    info(DEBUG, "translation finished.\n");

    info(DEBUG, "target NuSMV file writing ...");
    prosmv();
    info(DEBUG, "NuSMV file finished.\n");

    fclose(yyin);
    end = clock();
    costTime = (double)(end - begin)/CLOCKS_PER_SEC;
    allDone(0);
}
