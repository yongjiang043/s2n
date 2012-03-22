#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "y.tab.h"
#include "parse.h"
#include "spinlex.h"
#include "main.h"
#include "func.h"

#define MAXE 120

const char *cbit = "bit";
const char *cunsigned = "unsigned";
const char *cbyte = "byte";
const char *cshort = "short";
const char *cint = "int";
const char *cchan = "chan";
const char *cmtype = "mtype";

static FILE *mfile;

static int Lastpara = 0;    /*check whether last parameter of proctype decl*/
static int ground = 0;      /* counting for the label*/
static int space = 0;
static int sep_l = 0;
static int inif = 0;
static int intopro = 0;
static int decl_end = 0;    /* for testing if variable declare at start of proctype */
static int hasstatement = 0;   /* for testing if proctype just has variable decl without statements */

static void putSeq(Sequence *);
static void putChanArgs(Lextok *, int);

void putEnum(FILE *f)
{
    Lextok *pp;
    char *ename;

    fprintf(f, "{ ");
    for (pp = Mtype; pp; pp = pp->rgt)
    {
            ename = pp->lft->sym->name;
            fprintf(f, ename);
            if(pp->rgt) fprintf(f, ", ");
    }
    fprintf(f, " };");
}

/*
 * find mtype const with the id
 */
char *translateIntToMtype(int cnt)
{
    Lextok *le, *pe;
    int cn = 1;

    if (cnt <= 0)
    {
        info(WARN, "Value of mtype variable is LE than zero, which will be replaced by 1");
        cnt = 1;
    }
    for(pe = le = Mtype; le; pe = le, le = le->rgt)
    {
        if (cn == cnt)
        {
            return le->lft->sym->name;
        }
        cn++;
    }

    info(WARN, "value of mtype variable is out of range, which will be replaced by the max bound");
    return pe->lft->sym->name;
}

static int mprintf(const char *pattern,...)
{
    va_list arg_ptr;

    va_start(arg_ptr, pattern);

    vfprintf(mfile, pattern, arg_ptr);

    va_end(arg_ptr);

    return 0;
}

static int proMark()
{
    if (intopro)
    {
        ground = 0; intopro = 0;
    }
    ground++;

    return ground;
}

/*
 * output Mtype without mark
 */
static void mMtype()
{
    if(!Mtype) return;

    mprintf("mtype = ");
    putEnum(mfile);
    mprintf("\n\n");
}

const char *translateIntToType(unsigned short type)
{
    switch (type)
    {
        case BIT :       return cbit;
        case UNSIGNED :  return cunsigned;
        case BYTE :      return cbyte;
        case SHORT :     return cshort;
        case INT :       return cint;
        case CHAN :      return cchan;
        case MTYPE :     return cmtype;
        default :        return cint;
    }
}

static void putExpr(Lextok *exp)
{
    switch(exp->ntyp)
    {
        case CONST :
        {
            if (exp->ismtyp)
            {
                mprintf(translateIntToMtype(exp->val));
            }
            else
            {
                mprintf( "%d", exp->val);
            }
            break;
        }
        case NAME :
        {
            mprintf(exp->sym->name);
            if (exp->lft)
            {
                mprintf("[");
                putExpr(exp->lft);
                mprintf("]");
            }
            break;
        }
        case '(' :
        {
            mprintf("("); putExpr(exp->rgt);
            mprintf(")"); break;
        }
        case '+' :
        {
            putExpr(exp->lft); mprintf(" + ");
            putExpr(exp->rgt); break;
        }
        case '-' :
        {
            putExpr(exp->lft); mprintf(" - ");
            putExpr(exp->rgt); break;
        }
        case '*' :
        {
            putExpr(exp->lft); mprintf(" * ");
            putExpr(exp->rgt); break;
        }
        case '/' :
        {
            putExpr(exp->lft); mprintf(" / ");
            putExpr(exp->rgt); break;
        }
        case '%' :
        {
            putExpr(exp->lft); mprintf(" % ");
            putExpr(exp->rgt); break;
        }
        case '&' :
        {
            putExpr(exp->lft); mprintf(" & ");
            putExpr(exp->rgt); break;
        }
        case '^' :
        {
            putExpr(exp->lft); mprintf(" ^ ");
            putExpr(exp->rgt); break;
        }
        case '|' :
        {
            putExpr(exp->lft); mprintf(" | ");
            putExpr(exp->rgt); break;
        }
        case GT :
        {
            putExpr(exp->lft); mprintf(" > ");
            putExpr(exp->rgt); break;
        }
        case LT :
        {
            putExpr(exp->lft); mprintf(" < ");
            putExpr(exp->rgt); break;
        }
        case GE :
        {
            putExpr(exp->lft); mprintf(" >= ");
            putExpr(exp->rgt); break;
        }
        case LE :
        {
            putExpr(exp->lft); mprintf(" <= ");
            putExpr(exp->rgt); break;
        }
        case EQ :
        {
            putExpr(exp->lft); mprintf(" == ");
            putExpr(exp->rgt); break;
        }
        case NE :
        {
            putExpr(exp->lft); mprintf(" != ");
            putExpr(exp->rgt); break;
        }
        case AND :
        {
            putExpr(exp->lft); mprintf(" && ");
            putExpr(exp->rgt); break;
        }
        case OR :
        {
            putExpr(exp->lft); mprintf(" || ");
            putExpr(exp->rgt); break;
        }
        case LSHIFT :
        {
            putExpr(exp->lft); mprintf(" << ");
            putExpr(exp->rgt); break;
        }
        case RSHIFT :
        {
            putExpr(exp->lft); mprintf(" >> ");
            putExpr(exp->rgt); break;
        }
        case '~' :
        {
            mprintf("~ ");
            putExpr(exp->lft); break;
        }
        case UMIN :
        {
            mprintf("-");
            putExpr(exp->lft); break;
        }
        case '!' :
        {
            mprintf("!");
            putExpr(exp->lft);break;
        }
        case '?' :
        {
            putExpr(exp->lft);
            mprintf(";"); putExpr(exp->rgt->lft);
            mprintf(" : "); putExpr(exp->rgt->rgt);
            break;
        }
        case FULL :
        {
            mprintf("full(",mfile); putExpr(exp->lft);
            mprintf(")"); break;
        }
        case NFULL :
        {
            mprintf("nfull("); putExpr(exp->lft);
            mprintf(")"); break;
        }
        case EMPTY :
        {
            mprintf("empty("); putExpr(exp->lft);
            mprintf(")"); break;
        }
        case NEMPTY :
        {
            mprintf("nempty("); putExpr(exp->lft);
            mprintf(")"); break;
        }
        case RUN :
        {
            addRun(exp);
            mprintf("run %s(", exp->sym->name);
            putChanArgs(exp->lft, '!');
            mprintf(")"); break;
        }
    }
}

static void putChan(Lextok *chandel)
{
    Lextok *tylist;

    mprintf("[%d] of {", chandel->val);

    for (tylist = chandel->rgt; tylist; tylist = tylist->rgt)
    {
        mprintf(translateIntToType(tylist->ntyp));
        if (tylist->rgt)
        {
            mprintf(", ");
        }
    }

    mprintf("}");
}

static void putVarDec(Lextok *tn)
{
    mprintf(" %s", tn->sym->name);

    if (!(tn->sym->isarray) && tn->sym->nbits > 0)
    {
        mprintf(" : %d", tn->sym->nbits);
    }

    if (tn->sym->isarray == 1)
    {
        mprintf("[%d]", tn->sym->nel);
    }

    if (tn->sym->ini)
    {
        if (tn->sym->ini->ntyp == CHAN)
        {
            mprintf(" = ");
            putChan(tn->sym->ini);
        }
        else
        {
            mprintf(" = ");
            putExpr(tn->sym->ini);
        }
    }

    if (tn->rgt)
    {
        mprintf(",");
    }
    else if (!Lastpara)         /*last parameter*/
    {
        mprintf(";");
    }
}

static void putDec(Lextok *now)
{
    Lextok *turn;

    if (!(now->ntyp) || now->ntyp != TYPE)
    {
        lineno = now->ln;
        info(ERROR, "bad varible declaration! -- cannot happen");
    }
    else
    {
        mprintf(translateIntToType(now->sym->type));
        for (turn = now; turn; turn = turn->rgt)
        {
             putVarDec(turn);
        }
    }
}

/*
 * output global declaration
 */
static void mGlobalVar()
{
    LexList *pt; Lextok *now;

    if (!Gtype) return;

    for (pt = Gtype; pt; pt = pt->next)
    {
        now = pt->this;
        putDec(now);
        mprintf("\n");
    }
    mprintf("\n");
}

//--------------------Plist------------------------------
static void putActive(ProcList *pc)        /*output active clause*/
{
    if(pc->pronum == 1)
    {
        mprintf("active ");
    }
    if(pc->pronum > 1)
    {
        mprintf("active[%d] ", pc->pronum);
    }

    return;
}

static void putParameter(Lextok *para)        /*output parameter clause of proctype */
{
    Lextok *pt;

    mprintf("(");

    if(para)
    {
        for(pt = para; pt; pt = pt->rgt)
        {
            if(pt->ntyp != ',')
            {
                info(ERROR, "parameter of proctype is wrong!");
            }
            if(! pt->rgt)
            {
                Lastpara = 1;
            }
            putDec(pt->lft);
        }
    }

    mprintf(")");
    Lastpara = 0;
}

/*
 * output proctype body labeled
 */
static void mProclist()
{
    ProcList *pc;
    for(pc = Plist; pc; pc = pc->nxt)
    {
        context = pc->n;
        intopro = 1;
        if(!strcmp(pc->n->name,":init:"))
        {
            mprintf("init\n");
        }
        else
        {
            putActive(pc);
            mprintf("proctype %s ", pc->n->name);
            putParameter(pc->p);

            if (pc->prov)
            {
                mprintf(" provided ( ");
                putExpr(pc->prov);
                mprintf(" )");
            }
            mprintf("\n");
        }
        space = space + 6;
        mprintf("{\n"); putSeq(pc->s); mprintf("}\n");
        if(!hasstatement)
        {
            info(ERROR, "At least one statement in a proctype");
        }
        else
        {
            hasstatement = 0;
        }
        space = space - 6;
        pc->max_mark = ground;
        mprintf("\n");
        decl_end = 0;
    }
}

//-------------------------Sequence------------------------------------

static void putMark(Lextok *lx)
{
    if(!lx->lab) return;

    mprintf("M%-3d@ ",lx->lab);
}

static void putReceiveArg(Lextok *ra)
{
    if (ra->ntyp == NAME)
    {
        mprintf(ra->sym->name);
    }

    if (ra->ntyp == EVAL)
    {
        mprintf("eval( ");
        putExpr(ra->lft);
        mprintf(" )");
    }

    if (ra->ntyp ==CONST)
    {
        if (ra->ismtyp)
        {
            mprintf(translateIntToMtype(ra->val));
        }
        else
        {
            mprintf("%d", ra->val);
        }
    }
}

int isSimpleStm(Lextok *stm)
{
    int i = 0;

    switch(stm->ntyp)
    {
        case ASGN:
        case 's':
        case 'r':
        case 'c':
        case ELSE:
        case GOTO: i = 1; break;
        default : break;
    }
    return i;
}

static void putChanArgs(Lextok *rar,int i)
{
    Lextok *ar;

    for(ar = rar; ar && ar->ntyp == ','; ar = ar->rgt)
    {
        if(i == '?')
        {
            putReceiveArg(ar->lft);
        }
        if(i == '!')
        {
            putExpr(ar->lft);
        }
        if (ar->rgt)
        {
            mprintf(", ");
        }
    }
}

static void putIoOfChan(Lextok *io, int tp)
{
    if(!io || !io->sym)
    {
        return;
    }

    mprintf(io->sym->name);
    if (tp == '?')
    {
        mprintf(" ? "); putChanArgs(io->rgt, '?');
    }
    if (tp == '!')
    {
        mprintf(" ! "); putChanArgs(io->rgt, '!');
    }
}

static void putIfOrDo(Lextok *iff)
{
    SeqList *opt;

    if (iff->ntyp == IF)
    {
        mprintf("if\n");
    }
    if (iff->ntyp == DO)
    {
        mprintf("do\n");
    }

    space = space + 6;

    for (opt = iff->sl; opt; opt = opt->nxt)
    {
        nspace(space, mfile);
        space = space+3;
        mprintf(":: ",mfile); sep_l = 1;
        putSeq(opt->this);
        space = space - 3;
    }

    nspace(space,mfile);

    if (iff->ntyp == IF)
    {
        mprintf("fi");
    }
    if (iff->ntyp == DO)
    {
        mprintf("od");
    }

    space = space - 6;
}

/*
 * marking stmnts and ouput them
 */
static void putStm(Element *ele)
{
    Lextok *stm;
    Symbol *label;
    Element *e;
    int semi = 1, enter = 1;    /* semi for ';', enter for '\n' */

    stm = ele->n;
    if (!stm || stm->ntyp == '.' || stm->ntyp == BREAK || stm->ntyp == 0)
    {
        return;
    }

    if (!sep_l)
    {
        nspace(space, mfile);    /* if behind :: */
    }

    if (isSimpleStm(stm) || stm->ntyp == IF || stm->ntyp == DO)
    {
        stm->lab = proMark();
        /*
        if(stm->ntyp == 's' && stm->sym->rev)
        {
            ground++;
        }*/
        putMark(stm);
    }

    if (label = getLabel(ele))
    {
        mprintf("%s: ",label->name);
    }

    switch(stm->ntyp)
    {
        case 'c' :
        {
            stm->lft->lab = stm->lab;
            putExpr(stm->lft); break;
        }
        case 'r' :
        {
            putIoOfChan(stm, '?'); break;
        }
        case 's' :
        {
            putIoOfChan(stm, '!'); break;
        }
        case IF  :
        case DO  :
        {
            putIfOrDo(stm); break;
        }
        case ASGN :
        {
            mprintf(stm->sym->name);
            if(stm->lft->lft)
            {
                mprintf("[");
                putExpr(stm->lft->lft);
                mprintf("]");
            }
            mprintf(" = ");
            putExpr(stm->rgt); break;
        }
        case GOTO :
        {
            e = getElementWithLabel(stm, 1);
            crossDsteps(stm, e->n);
            if (!stm->rgt)
            {
                mprintf("break");
            }
            else
            {
                mprintf("goto %s",stm->sym->name);
            }
            break;
        }
        case ELSE :
        {
            if (!sep_l)
            {
                lineno = stm->ln;
                info(ERROR, "misplace else statement");
            }
            mprintf("else"); break;
        }
        case UNLESS :
        {
            mprintf("{\n");
            space = space + 8;
            sep_l = 0;     putSeq(stm->sl->this);
            space = space - 8;
            nspace(space, mfile); mprintf("} unless\n");

            nspace(space, mfile); mprintf("{\n");
            space = space + 8;
            putSeq(stm->sl->nxt->this);
            space = space - 8;
            nspace(space, mfile); mprintf("}");

            semi = 0;
            e = stm->sl->this->frst;
            while (e->n->ntyp == '.' || e->n->ntyp == 0)
            {
                e = e->nxt;
            }
            stm->lab = e->n->lab;
            break;
        }
        case D_STEP :
        case ATOMIC :
        {
            mprintf("%s {\n", (stm->ntyp == D_STEP)?"d_step":"atomic");

            sep_l = 0;
            space = space + 8;
            putSeq(stm->sl->this);
            space = space - 8;

            nspace(space, mfile); mprintf("}");

            e = stm->sl->this->frst;
            while (e->n->ntyp == '.' || e->n->ntyp == 0)
            {
                e = e->nxt;
            }
            if(!stm->inatom)
            {
                e->n->inatom = 0;           // eliminate the atomical
            }
            stm->lab = e->n->lab;
            break;
        }
        case NON_ATOMIC :
        {
            info(WARN, "non_atomic in marking!-----cannot happen\n");
            break;
        }
        default :
        {
            info(WARN, "type %d lextok in putStm.\n", stm->ntyp);
        }
    }

    if (semi)
    {
        mprintf(";", mfile);
    }
    if (enter)
    {
        mprintf("\n", mfile);
    }

    sep_l = 0;
}

static void putStep(Element *ele)
{
    Lextok *st;

    if (!ele)
    {
        return;
    }

    st = ele->n;
    if (st->ntyp == TYPE)
    {
        if(decl_end)
        {
            info(ERROR, "variable declare should be at start of proctype");
        }
        nspace(space, mfile);
        putDec(st);
        mprintf("\n");
    }
    else
    {
        hasstatement = 1;
        decl_end = 1;
        putStm(ele);
    }
}

static void putSeq(Sequence *seq)
{
    Element *el;

    if(!seq)
    {
        return;
    }

    for(el = seq->frst; el && el->n->ntyp != '@'; el = el->nxt)
    {
        putStep(el);

        if (el == seq->extent)
        {
            break;
        }
    }
}

void marking()
{
    char *filename;
    int  leng = strlen(srcfile);

    filename = (char *)emalloc(leng + 8);
    sstrcpy(filename, srcfile, 0, leng-5);
    strcat(filename, "_mark.pml");
 
    if (!(mfile = fopen(filename, "w")))
    {
        info(ERROR, "cannot open %s\n", filename);
    }

    initRunMap();

    mMtype();
    mGlobalVar();
    mProclist();

    topoSortRunMap();

    if (!ifmark)
    {
        (void)unlink(filename);
        info(NOTE, "your marked spin source hasnot been saved.\nIf you want to save it, use -m or -M.");
    }
    else
    {
        info(NOTE, "your marked spin source has been saved as %s.", filename);
    }
    fclose(mfile);
}
