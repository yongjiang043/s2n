#include <stdlib.h>
#include <stdio.h>
#include "spin.h"
#include "main.h"
#include "func.h"
#include "y.tab.h"
#include "spinlex.h"

Symbol   *context = ZS;
Label    *labtab = (Label *)0;
Lextok   *Mtype = (Lextok *)0;
LexList  *Gtype = (LexList *)0;
ProcList *Plist = (ProcList *)0;
SeqList  *cur_s = (SeqList  *)0;
DynamicList *topoOrder = (DynamicList *)0;

static int Unique = 0, DstepStart = -1;
static int break_id = 0;
static Symbol   *symtab[Nhash+1];
static Lextok   *innermost;
static Lbreak   *breakstack = (Lbreak *)0;
static DynamicList *runMap = (DynamicList *)0;          // point to map of synamic runs
static DynamicList *stack = (DynamicList *)0;

static Element *newElement(Lextok *);
static void attach_escape(Sequence *, Sequence *);

static int sameName(Symbol *a, Symbol *b)
{
    if (!a && !b)
    {
        return 1;
    }
    if (!a || !b)
    {
        return 0;
    }

    return !strcmp(a->name, b->name);
}

int hash(char *s)
{
    int h=0;

    while (*s)
    {
        h += *s++;
        h <<= 1;
        if (h&(Nhash+1))
        {
            h |= 1;
        }
    }

    return h&Nhash;
}

Lextok *nn(Lextok *s, int t, Lextok *ll, Lextok *rl)
{
    Lextok *n = (Lextok *) emalloc(sizeof(Lextok));

    n->ntyp = (short) t;
    if (s)
    {
        n->ln = s->ln;
    }
    else if (rl)
    {
        n->ln = rl->ln;
    }
    else if (ll)
    {
        n->ln = ll->ln;
    }
    else
    {
        n->ln = lineno;
    }

    if (s)
    {
        n->sym = s->sym;
    }
    n->lft = ll;
    n->rgt = rl;
    n->indstep = DstepStart;

    return n;
}

Symbol *lookup(char *s)
{
    Symbol *sp;
    int h = hash(s);

    for (sp = symtab[h]; sp ; sp = sp->next)      // same local or same global
    {
        if (strcmp(sp->name,s)==0
            && sameName(sp->context, context))
        {
            return sp;
        }
    }

    if (context)
    {
        for (sp = symtab[h]; sp; sp = sp->next)       // global
        {
            if (strcmp(sp->name, s)==0
                && !sp->context)
            {
                return sp;
            }
        }
    }
    sp = (Symbol *) emalloc(sizeof(Symbol));
    sp->name = (char *) emalloc(strlen(s) + 1);
    strcpy(sp->name, s);
    sp->nel = 1;
    sp->context = context;

    sp->next = symtab[h];
    symtab[h] = sp;
/*
    no = (Ordered *) emalloc(sizeof(Ordered));
    no->entry = sp;

    if (!last_name)
    {
        last_name = all_names = no;
    }
    else
    {
        last_name->next = no;
        last_name = no;
    }
*/
    return sp;
}

void setMtype(Lextok *m)
{
    Lextok *n;
    int cnt, oln = lineno;

    if(m)
    {
        lineno = m->ln;
    }

    if(!Mtype)
    {
        Mtype = m;
    }
    else
    {
        for (n = Mtype; n->rgt; n = n->rgt)
            ;
        n->rgt = m;
    }

    for (n = Mtype, cnt = 1; n; n=n->rgt,cnt++)
    {
        if(!n->lft || !n->lft->sym
           || n->lft->ntyp != NAME
           || n->lft->lft)
        {
            info(ERROR, "bad mtype definition");
        }

        if(n->lft->sym->type != MTYPE)
        {
            // n->lft->sym->hidden |= 128;
            n->lft->sym->type = MTYPE;
            n->lft->sym->ini = nn(ZN,CONST,ZN,ZN);
            n->lft->sym->ini->val = cnt;
        }
        else if (n->lft->sym->ini->val !=cnt)
        {
            info(WARN, "name %s appears twice in mtype declaration", n->lft->sym->name);
        }
    }
    lineno = oln;
    if (cnt > 256)
    {
        info(ERROR, "too many mtype elements (>255)");
    }
} 

void checkVarDeclare(Lextok *n, int t)
{
    int oln = lineno;
    Lextok *m;

    m = n;
    while(m)
    {
        lineno = m->ln;

        if (m->sym->type/* && !(m->sym->hidden&32)*/)
        {
            info(ERROR, "redeclaration of '%s'", m->sym->name);
        }
        m->sym->type = (short) t;

        if(t == UNSIGNED)
        {
            /*
            if (m->sym->nbits < 0 || m->sym->nbits >= 32)
            {
                info(ERROR, "unsigned variable's bits (n) should 0<=n<32, (%s) has invalid width-field", m->sym->name);
            }
            */
            if (m->sym->nbits == 0)
            {
                m->sym->nbits = 16;
                info(WARN, "unsigned without width-field, default by 16");
            }
        }
        else if (m->sym->nbits > 0)
        {
            info(WARN, "(%s) only an unsigned can have width-field", m->sym->name);
            m->sym->nbits = 0;
        }

//      if (t == CHAN)
//          m->sym->Nid = ++Nid;

        if (t != CHAN)
        {
            if (m->sym->ini && m->sym->ini->ntyp == CHAN)
            {
                info(ERROR, "chan initializer for non-channel %s", m->sym->name);
            }
        }
        /*
        if (m->sym->nel <= 0)
        {
            info(WARN, "bad array size for '%s'", m->sym->name);
            m->sym->nel = 1;
        }
        */
        m = m->rgt;
    }
    lineno = oln;
}

void setGloberVar(Lextok *n)
{
    LexList *t,*tmp;

    tmp = (LexList *)emalloc(sizeof(LexList));
    tmp->this = n;
    if (!Gtype)
    {
        Gtype = tmp;
    }
    else
    {
        for(t = Gtype; t->next; t=t->next)
            ;
        t->next = tmp;
    }
}

int properEnabler(Lextok *n)
{
    if (!n)
    {
        return 1;
    }
    switch(n->ntyp)
    {
        case NEMPTY: case FULL:
        case NFULL:  case EMPTY:
        case NAME:
            return (!(n->sym->context));

        case CONST:
            return 1;
        case '!': case UMIN: case '~':
            return properEnabler(n->lft);

        case '/': case '*': case '-': case '+':
        case '%': case LT: case GT: case '&': case '^':
        case '|': case LE: case GE: case NE: case '?':
        case EQ: case OR: case AND: case LSHIFT:
        case RSHIFT: case 'c':
            return properEnabler(n->lft) && properEnabler(n->rgt);
        default:
            break;
    }
    printf("s2n: saw ");
    explain(n->ntyp);
    printf("\n");

    return 0;
}

void checkProName(Lextok *m)
{
    ProcList  *tmp;

    for (tmp = Plist; tmp; tmp = tmp->nxt)
    {
        if (!strcmp(tmp->n->name, m->sym->name))
        {
            info(ERROR, "proctype %s redefined", m->sym->name);
        }
    }
}

int isProctype(char *t)
{
    ProcList *tmp;

    for(tmp = Plist; tmp; tmp = tmp->nxt)
    {
        if (!strcmp(tmp->n->name, t))
        {
            return 1;
        }
    }

    return 0;
}

ProcList *setProcList(Symbol *n, Lextok *p, Sequence *s, Lextok *prov)
{
    ProcList *r = (ProcList *) emalloc(sizeof(ProcList));
    ProcList *pl;

    r->n = n;
    r->p = p;
    r->s = s;
    r->prov = prov;

    if (!Plist)
    {
        Plist = r;
    }
    else
    {
        for (pl = Plist; pl->nxt; pl = pl->nxt)
            ;
        pl->nxt = r;
    }

    return r;
}

int isMtype(char *str)
{
    Lextok *n;
    int cnt = 1;

    for (n = Mtype; n; n = n->rgt)
    {
        if(strcmp(str, n->lft->sym->name) == 0)
        {
            return cnt;
        }
        cnt++;
    }

    return 0;
}

void remSeq()
{
    DstepStart = Unique;
}

void unRemSeq()
{
    DstepStart = -1;
}

static void setLabel(Symbol *s, Element *e)
{
    Label *l;

    if (!s) return;
    for (l = labtab; l; l = l->nxt)
    {
        if (l->s == s && l->c == context)
        {
             info(ERROR, "label %s redeclared", s->name);
        }
    }

    l = (Label *)emalloc(sizeof(Label));
    l->s = s;
    l->c = context;
    l->e = e;
    l->nxt = labtab;
    labtab = l;
}

Element *getElementWithLabel(Lextok *n, int md)
{
    Label *l;
    Symbol *s = n->sym;

    for (l = labtab; l; l = l->nxt)
    {
        if (s == l->s /*&& s->context == l->c*/)
        {
            return l->e;
        }
    }

    lineno = n->ln;
    if (md)
    {
        info(ERROR, "undefined label %s", s->name);
    }

    return ZE;
}

Symbol *getLabel(Element *e)
{
    Label *l;

    for (l = labtab; l; l = l->nxt)
    {
        if (e != l->e)
        {
            continue;
        }
        else
        {
            return l->s;
        }
    }

    return ZS;
}

static int hasChanRef(Lextok *n)
{
    if (!n)
    {
        return 0;
    }

    switch(n->ntyp)
    {
        case 's': case 'r':
        case FULL: case NFULL:
        case EMPTY: case NEMPTY:
            return 1;

        default:
            break;
    }
    if (hasChanRef(n->lft))
    {
        return 1;
    }

    return hasChanRef(n->rgt);
}

static int jumpsLocal(Element *q, Element *stop)
{
    Element *lb, *f;
    SeqList *h;

    /* allow no jumps out of a d_step sequence */
    for (f = q; f; f = f->nxt)
    {
        if (f && f->n && f->n->ntyp == GOTO)
        {
            lb = getElementWithLabel(f->n, 0);
            if (!lb || lb->eleno < DstepStart)
            {
                return f->n->ln;
            }
        }
        for (h = f->sub; h; h = h->nxt)
        {
            if (!jumpsLocal(h->this->frst, h->this->last))
            {
                return f->n->ln;
            }
        }
        if(f == stop)
        {
            break;
        }
    }

    return 0;
}

void crossDsteps(Lextok *a, Lextok *b)
{
    if (a && b && b->indstep != -1
         &&  a->indstep != b->indstep)
    {
        info(WARN, "line %d: jump into d_step sequence", a->ln);
    }
}

int isSkip(Lextok *n)
{
    return (n->ntyp == PRINT
         || n->ntyp == PRINTM
         || (n->ntyp == 'c'
             && n->lft
             && n->lft->ntyp == CONST
             && n->lft->val  == 1));
}

void checkSequence(Sequence *s)
{
    Element *e, *le = ZE;
    Lextok *n;
    int cnt = 0;

    for (e = s->frst; e; le = e, e = e->nxt)
    {
        n = e->n;
        if (isSkip(n) && !getLabel(e))
        {
            cnt++;
            if (cnt > 1
               && n->ntyp != PRINT
               && n->ntyp != PRINTM)
            {
                info(WARN, "line %d, redundant skip\n", n->ln);
                if (e != s->frst && e != s->last && e != s->extent)
                {
                    le->nxt = e->nxt;
                    e = le;
                }
            }
        }
        else
        {
            cnt = 0;
        }
    }
}

void checkOptions(Lextok *n)
{
    SeqList *l;

    if (!n)
    {
        return;
    }

    for (l = n->sl; l; l = l->nxt)
    {
         checkSequence(l->this);
    }
}

Sequence *closeSeq(int nottop)
{
    Sequence *s = cur_s->this;
    Symbol *z;
    SeqList *sl;
    Element *e;

    for (e = s->frst; e && (e->n->ntyp == '.' || e->n->ntyp == 0); e = e->nxt)
        ;

    if (nottop > 0 && (z = getLabel(e)))
    {
        printf("error: (line:%d) label %s placed incorrectly\n",
            (s->frst->n)?s->frst->n->ln:0,
            z->name);
        switch (nottop)
        {
            case 1:
            {
                printf("=====> stmnt unless Label: stmnt\n");
                printf("sorry, cannot jump to the guard of an\n");
                printf("escape (it is not a unique state)\n");
                break;
            }
            case 2:
            {
                printf("=====> instead of  ");
                printf("\"Label: stmnt unless stmnt\"\n");
                printf("=====> always use  ");
                printf("\"Label: { stmnt unless stmnt }\"\n");
                break;
            }
            case 3:
            {
                printf("=====> instead of  ");
                printf("\"atomic { Label: statement ... }\"\n");
                printf("=====> always use  ");
                printf("\"Label: atomic { statement ... }\"\n");
                break;
            }
            case 4:
            {
                printf("=====> instead of  ");
                printf("\"d_step { Label: statement ... }\"\n");
                printf("=====> always use  ");
                printf("\"Label: d_step { statement ... }\"\n");
                break;
            }
            case 5:
            {
                printf("=====> instead of  ");
                printf("\"{ Label: statement ... }\"\n");
                printf("=====> always use  ");
                printf("\"Label: { statement ... }\"\n");
                break;
            }
            case 6:
            {
                printf("=====>instead of\n");
                printf("    do (or if)\n");
                printf("    :: ...\n");
                printf("    :: Label: statement\n");
                printf("    od (of fi)\n");
                printf("=====>always use\n");
                printf("Label:    do (or if)\n");
                printf("    :: ...\n");
                printf("    :: statement\n");
                printf("    od (or fi)\n");
                break;
            }
            case 7:
            {
                printf("cannot happen - labels\n");
                break;
            }
        }
        allDone(1);
    }

    sl = cur_s;
    cur_s = cur_s->nxt;
    free(sl);
    s->extent = s->last;

    if (!s->last)
    {
        info(ERROR, "sequence must have at least one statement");
    }

    return s;
}

void jumpOutOfDstep(Sequence *s)
{
    int oln;

    if (oln = jumpsLocal(s->frst, s->last))
    {
        info(WARN, "lineno %d: non_local jump in d_step sequence", oln);
    }
}
void pushBreak()
{
    char buf[64];
    Lbreak *r = (Lbreak *)emalloc(sizeof(Lbreak));
    Symbol *l;

    sprintf(buf, ":b%d", break_id++);
    l = lookup(buf);
    r->l = l;
    r->nxt = breakstack;
    breakstack = r;
}

Symbol *popBreak()
{
    if (!breakstack)
    {
        info(ERROR, "misplaced break statement");
    }

    return breakstack->l;
}

SeqList *addSeqToSeqlist(Sequence *s, SeqList *r)
{
    SeqList *t = (SeqList *) emalloc(sizeof(SeqList));

    t->this = s;
    t->nxt = r;

    return t;
}

void addElementToSeq(Element *e, Sequence *s)
{
    if (!s->frst)
    {
        s->frst = e;
    }
    else
    {
        s->last->nxt = e;
    }

    s->last = e;
}

static Element *addIfOrDoLextokToSeq(Lextok *n)
{
    int oln = lineno;
    int tok = n->ntyp;
    int ref_chans = 0;
    SeqList *s = n->sl;
    Element *e = newElement(ZN);
    Element *t = newElement(nn(ZN, '.', ZN, ZN));
    SeqList *z, *prev_z = (SeqList *)0;
    SeqList *move_else  = (SeqList *)0;
    Lbreak  *lb;

    for (z = s; z; z = z->nxt)
    {
        if (!z->this->frst)
        {
            continue;
        }
        if (z->this->frst->n->ntyp == ELSE)
        {
            if (move_else)
            {
                lineno = z->this->frst->n->ln;
                info(ERROR, "duplicate 'else'");
                lineno = oln;
            }
            if (z->nxt)
            {
                move_else = z;
                if (prev_z)
                {
                    prev_z->nxt = z->nxt;
                }
                else
                {
                    s = n->sl = z->nxt;
                }
                continue;
            }
        }
        else
        {
            ref_chans |= hasChanRef(z->this->frst->n);
        }
        prev_z = z;
    }

    if (move_else)
    {
        move_else->nxt = (SeqList *)0;
        if (!prev_z)
        {
            info(ERROR, "cannot happen - if_seq");
        }
        prev_z->nxt = move_else;
        prev_z = move_else;
    }

    if (prev_z
       && ref_chans
       && prev_z->this->frst->n->ntyp == ELSE)
    {
        prev_z->this->frst->n->val = 1;
        lineno = prev_z->this->frst->n->ln;
        info(WARN, "dubious use of 'else' combined with channel i/o.");
        lineno = oln;
    }

    e->n = n;
    e->sub = s;
    for(z = s; z; prev_z = z, z = z->nxt)
    {
        addElementToSeq(t, z->this);
    }

    if (tok == DO)
    {
        addElementToSeq(t, cur_s->this);
        t = newElement(nn(n, BREAK, ZN, ZN));
        setLabel(popBreak(), t);
        lb = breakstack;
        breakstack = breakstack->nxt;
        free(lb);
    }
    addElementToSeq(e, cur_s->this);
    addElementToSeq(t, cur_s->this);

    return e;
}

static void
escape_el(Element *f, Sequence *e)
{
    SeqList *z;

    for (z = f->esc; z; z = z->nxt)
    {
        if (z->this == e)
        {
             return; /* already there */
        }
    }

    /* cover the lower-level escapes of this state */
    for (z = f->esc; z; z = z->nxt)
    {
        attach_escape(z->this, e);
    }

    /* now attach escape to the state itself */

    f->esc = addSeqToSeqlist(e, f->esc);    /* in lifo order... */

    switch (f->n->ntyp)
    {
        case UNLESS:
        {
            attach_escape(f->sub->this, e);
            break;
        }
        case IF:
        case DO:
        {
            for (z = f->sub; z; z = z->nxt)
            {
                attach_escape(z->this, e);
            }
            break;
        }
        case D_STEP:
        {        /* attach only to the guard stmnt */
            escape_el(f->n->sl->this->frst, e);
            break;
        }
        case ATOMIC:
        case NON_ATOMIC:
        {        /* attach to all stmnts */
            attach_escape(f->n->sl->this, e);
            break;
        }
        default:
        {
            break;
        }
    }
}

static void
attach_escape(Sequence *n, Sequence *e)
{
    Element *f;

    for (f = n->frst; f; f = f->nxt)
    {
        escape_el(f, e);
        if (f == n->extent)
        {
            break;
        }
    }
}

static Element *addUnlessLextokToSeq(Lextok *n)
{
    int oln = lineno;
    SeqList *s = n->sl;
    Element *e = newElement(ZN);
    Element *t = newElement(nn(ZN, '.', ZN, ZN));

    e->n = n;
    e->sub = s;

    if (!s || !s->nxt || s->nxt->nxt)
    {
        lineno = n->ln;
        info(ERROR, "unexpected unless structure");
    }

    attach_escape(s->this, s->nxt->this);

    for(; s; s = s->nxt)
    {
        addElementToSeq(t, s->this);
    }

    addElementToSeq(e, cur_s->this);
    addElementToSeq(t, cur_s->this);

    return e;
}

static Element *newElement(Lextok *n)
{
    Element *m;

    if (n)
    {
        if (n->ntyp == IF || n->ntyp == DO)
        {
            return addIfOrDoLextokToSeq(n);
        }
        if (n->ntyp == UNLESS)
        {
            return addUnlessLextokToSeq(n);
        }
    }
    m = (Element *)emalloc(sizeof(Element));
    m->n = n;
    m->eleno = Unique++;

    return m;
}

Element *colons(Lextok *n)
{
    if (!n)
    {
        return ZE;
    }
    if (n->ntyp == ':')
    {
        Element *e = colons(n->rgt);
        setLabel(n->sym, e);

        return e;
    }
    innermost = n;

    return newElement(n);
}

void addLextokToSeq(Lextok *n)
{
    Element *e;
    Element *t = newElement(nn(ZN, '.', ZN, ZN));

    if (!n)
    {
        return;
    }

    innermost = n;
    e = colons(n);
    if (innermost->ntyp != IF
        && innermost->ntyp != DO
        && innermost->ntyp != UNLESS)
    {
        if (innermost->ntyp == ATOMIC || innermost->ntyp == NON_ATOMIC || innermost->ntyp == D_STEP)
        {
            addElementToSeq(t, innermost->sl->this);
            addElementToSeq(e, cur_s->this);
            addElementToSeq(t, cur_s->this);
        }
        else
        {
            addElementToSeq(e, cur_s->this);
        }
    }
}

void openSeq()
{
    SeqList *t;
    Sequence *s = (Sequence *) emalloc(sizeof(Sequence));

    t = addSeqToSeqlist(s,cur_s);
    cur_s = t;
}

void splitSequence(Lextok *seq)
{
    if (!seq || seq->ntyp != NON_ATOMIC || !seq->sl || !seq->sl->this || seq->sl->nxt )
    {
        printf("bad sequence ----cannot happen in splitSequence\n");
        return;
    }

    addElementToSeq(seq->sl->this->frst, cur_s->this);

    cur_s->this->last = seq->sl->this->extent;
}

Lextok *unlessLextok(Lextok *No, Lextok *Es)
{
    SeqList *Sl;
    Lextok *Re = nn(ZN, UNLESS, ZN, ZN);
    Re->ln = No->ln;

    if (Es->ntyp == NON_ATOMIC)
    {
        Sl = Es->sl;
    }
    else
    {
        openSeq();
        addLextokToSeq(Es);
        Sl = addSeqToSeqlist(closeSeq(1), 0);
    }

    if (No->ntyp == NON_ATOMIC)
    {
        No->sl->nxt = Sl;
        Sl = No->sl;
    }
    else if (No->ntyp == ':'
            && (No->rgt->ntyp == NON_ATOMIC
            ||  No->rgt->ntyp == ATOMIC
            ||  No->rgt->ntyp == D_STEP))
    {
        if (No->rgt->ntyp == NON_ATOMIC)
        {
            No->rgt->sl->nxt = Sl;
            Sl = No->rgt->sl;
            Re->sl = Sl;
        }
        if (No->rgt->ntyp == ATOMIC ||  No->rgt->ntyp == D_STEP)
        {
            openSeq();
            addLextokToSeq(No->rgt);
            Re->sl = addSeqToSeqlist(closeSeq(7), Sl);
        }

        Re = nn(No, ':', ZN, Re);
        Re->ln = No->ln;

        return Re;
    }
    else
    {
        openSeq();
        addLextokToSeq(No);
        Sl = addSeqToSeqlist(closeSeq(2), Sl);
    }

    Re->sl = Sl;

    return Re;
}

void checkAtomic(Sequence *s, int added)
{
    int oln;
    Element *f, *pre;
    SeqList *h;
    Element *a = s->frst, *b = s->last;

    for (pre = f = a; ; pre = f, f = f->nxt)
    {
        oln = f->n->ln;
        switch (f->n->ntyp)
        {
            case ATOMIC:
            {
                // atomic/d_step{... atomic{...}...} will be atomic/d_step{...}
                info(WARN, "lineno %d: atomic inside %s (ignored)", oln, (added)?"d_step":"atomic");
                h = f->n->sl;
                checkAtomic(h->this, added);
                goto mknonat;
            }
            case D_STEP:
            {
                h = f->n->sl;
                checkAtomic(h->this, added);
                if(added)
                {
                    // d_step{... d_step{..}...} will be d_step{...}
                    info(WARN, "lineno %d: d_step inside d_step (ignored)", oln);
                    goto mknonat;
                }
                else
                {
                    // atomic{... d_step{..}...} will not be changed but throw a warning
                    info(WARN, "lineno %d: d_step inside atomic", oln);
                    break;
                }
            }
            case NON_ATOMIC:
            {
                info(ERROR, "cannot happen -- in checkAtomic");
    mknonat:    if(f == a)
                {
                    s->frst = h->this->frst;
                }
                else
                {
                    pre->nxt = h->this->frst;
                }
                h->this->last = f->nxt;
                break;
            }
            case UNLESS:
            {
                for (h = f->sub; h; h = h->nxt)
                {
                    checkAtomic(h->this, added);
                }
                break;
            }
            default:
                break;
        }
        if (f == b)
        {
            break;
        }
    }
}

static DynamicList *seachHost(char *nameOfHost)
{
    DynamicList *result;

    result = runMap;
    while(result)
    {
        if(strcmp(result->host->n->name, nameOfHost) == 0)
        {
            return result;
        }

        result = result->nxt;
    }

    return 0;
}

static RunList *seachGuest(DynamicList *host, char *guest)
{
    RunList *pre, *result;

    pre = result = host->rl;
    while(result)
    {
        if(strcmp(result->guest, guest) == 0)
        {
            return result;
        }

        pre = result;
        result = result->nxt;
    }

    result = (RunList *)emalloc(sizeof(RunList));
    result->guest = guest;

    if(!host->rl)
    {
        host->rl = result;
    }
    else
    {
        pre->nxt = result;
    }

    return result;
}

static void addRunToHost(DynamicList *host, Lextok *run, char *guestName)
{
    DynamicList *tail;
    OneRun *item, *temp;
    RunList *hang;

    if(!(tail = seachHost(guestName)))
    {
        info(ERROR, "proctype %s in 'run' not declared", guestName);
    }
    tail->host->runable = 1;
    tail->indegree++;

    hang = seachGuest(host, guestName);
    item = (OneRun *)emalloc(sizeof(OneRun));
    item->r = run;

    if(!hang->run)
    {
        hang->run = item;
    }
    else
    {
        for(temp = hang->run; temp->nxt; temp = temp->nxt)
            ;
        temp->nxt = item;
    }
}

ProcList *lookupProcess(char *name)
{
    ProcList *p = Plist;

    for(; p; p = p->nxt)
    {
        if(strcmp(p->n->name, name) == 0)
        {
            break;
        }
    }

    return p;
}

static int countFormalPara(Lextok *p)
{
    int result = 0;
    Lextok *t1, *t2;

    for(t1 = p; t1 && t1->ntyp == ','; t1 = t1->rgt)
    {
        for(t2 = t1->lft; t2 && t2->ntyp == TYPE; t2 = t2->rgt)
        {
            result++;
        }
    }

    return result;
}

static int countActualPara(Lextok *p)
{
    int result = 0;

    for(; p && p->ntyp == ','; p = p->rgt)
    {
        result++;
    }

    return result;
}

static void checkValidRun(Lextok *run)
{
    int numOfFormalPara, numOfActualPara;
    ProcList *process;
    Lextok *ap, *fp;

    if(!(process = lookupProcess(run->sym->name)))
    {
        info(ERROR, "proctype %s not found", run->sym->name);
    }
    fp = process->p;
    numOfFormalPara = countFormalPara(fp);

    ap = run->lft;
    numOfActualPara = countActualPara(ap);

    if(numOfFormalPara != numOfActualPara)
    {
        info(ERROR, "number of your actual parameter and formal parameter are not the same");
    }
}

void addRun(Lextok *run)
{
    DynamicList *hostList;

    hostList = seachHost(context->name);
    addRunToHost(hostList, run, run->sym->name);
    checkValidRun(run);
}

void initRunMap()
{
    ProcList *p = Plist;
    DynamicList *d, *t;

    while(p)
    {
        d = (DynamicList *)emalloc(sizeof(DynamicList));
        d->host = p;

        if(!runMap)
        {
            runMap = d;
        }
        else
        {
            t->nxt = d;
        }
        t = d;
        p = p->nxt;
    }
}

static void printRunMap()
{
    DynamicList *d;
    RunList *r;
    OneRun *o;

    for(d = runMap; d; d = d->nxt)
    {
        printf("%s with indegree %d -->", d->host->n->name, d->indegree);
        for(r = d->rl; r; r = r->nxt)
        {
            printf("%s ", r->guest);
            for(o = r->run; o; o = o->nxt)
            {
                printf("%d ", o->r->lab);
                if(o->r->lft)
                {
                    printf("%d\n", o->r->lft->ntyp);
                }
            }
            printf(" | ");
        }
        printf("\n");
    }
}

static void printTopoOrder()
{
    DynamicList *d = topoOrder;

    printf("the topological order is\n");
    for(; d; d = d->nxtTopoEle)
    {
        printf(" %s", d->host->n->name);
    }
    printf("\n");
}

static void Push(DynamicList *ele)
{
    if(!stack)
    {
        stack = ele;
    }
    else
    {
        ele->nxtStackEle = stack;
        stack = ele;
    }
}

static DynamicList *Pop()
{
    DynamicList *result = 0;

    if(stack)
    {
        result = stack;
        stack = stack->nxtStackEle;
    }

    return result;
}

static void addToTopoOrder(DynamicList *ele)
{
    ele->nxtTopoEle = topoOrder;
    topoOrder = ele;
}

void topoSortRunMap()
{
    int numOfNode = 0, count = 0;
    DynamicList *p, *topoEle;
    RunList *r;
    OneRun *o;

//    printRunMap();    // debug

    for(p = runMap; p; p = p->nxt)
    {
        numOfNode++;
        if(!p->indegree)
        {
            Push(p);
        }
    }

    while(stack)
    {
        topoEle = Pop();
        addToTopoOrder(topoEle);
        count++;

        for(r = topoEle->rl; r; r = r->nxt)
        {
            p = seachHost(r->guest);
            for(o = r->run; o; o = o->nxt)
            {
                p->indegree--;
            }
            if(!p->indegree)
            {
                Push(p);
            }
            if(p->indegree < 0)
            {
                info(ERROR, "cannot happen -- topoSort");
            }
        }
    }

    if(count < numOfNode)
    {
//        info(DEBUG, "count = %d, numOfNode = %d", count, numOfNode);
        info(ERROR, "there should not be any cycle in map of run");
    }

//    printTopoOrder();     //debug
}
