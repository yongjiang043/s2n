#include <stdlib.h>
#include <stdio.h>
#include "spin.h"
#include "y.tab.h"
#include "func.h"
#include "smvsym.h"
#include "express.h"
#include "module.h"
#include "variable.h"
#include "parse.h"

static Express *unless_cond = 0;

static void addNext(Next *, Module *);
void buildNext(Element *, Module *);

/*
void
add_para(Module *mod, Varinf *var)
{
    Varinf *tv;
    VarList *sl, *slt;

    for(tv = var; tv; tv = tv->nxt)
    {    slt = (VarList *)emalloc(sizeof(VarList));
         slt->var = tv;
         if(!mod->paralist) mod->paralist = slt;
         else
         {   for(sl = mod->paralist; sl->next; sl = sl->next)
               ;  sl->next = slt;
         }
    }
}
*/
static Next *createNext(int nowpc, int idx, Varinf *lvar, Varinf *svar, Express *rval, Express *cond)
{
    Next *nt = (Next *)emalloc(sizeof(Next));

    nt->nowpc = nowpc;
    nt->idx = idx;
    nt->lvar = lvar;
    nt->svar = svar;
    nt->rval = rval;
    nt->cond = cond;

    if(nowpc)
    {
        checkTypeCompatible(nt);
    }

    return nt;
}

static int isArray(Lextok *lex)
{
    int i;

    if(lex->lft)
    {
        if (lex->lft->ntyp != CONST)
        {
            info(ERROR, "line %d, index of array should be const.", lex->ln);
        }
        else
        {
            i = lex->lft->val;
        }
    }
    else
    {
        i = '#';
    }

    return i;
}

static void addMultiNext(Next *ns, Module *mod)
{
    int i;
    int min = ns->lvar->Imin;
    int max = ns->lvar->Imax;
    Next *net;

   for(i = min; i <= max; i++)
   {
       net = createNext(ns->nowpc, i, ns->lvar, ns->svar, ns->rval, ns->cond);
       addNext(net,mod);
   }
}

static void addNext(Next *ns, Module *mod)
{
    NextList *slt, *last;
    Next *nst;

    if(!mod)
    {
        info(ERROR, "cannot happen --- addNext");
    }

//    if(sameContext(mod, ns->lvar->context))
//    {
        ns->lvar->hasnext = 1;
//    }

    if (ns->lvar->isarray && ns->idx == '#')
    {
        addMultiNext(ns,mod);
        return;
    }

    for(slt = mod->nextl, last = slt; slt; last = slt,slt = slt->nxt)
    {
        nst = slt->th;
        if (nst->lvar == ns->lvar && ns->idx == nst->idx && nst->svar == ns->svar)
        {
            for(; nst->nxt;nst = nst->nxt)
                        ;
            nst->nxt = ns;
            return;
        }
    }

    slt = (NextList *)emalloc(sizeof(NextList));
    slt->th = ns;
    if (!mod->nextl)
    {
        mod->nextl = slt;
    }
    else
    {
        last->nxt = slt;
    }
}

static Varinf *chanVar(Lextok *lex)
{
    Varinf *var;
    char *vname = (char *)emalloc(strlen(lex->sym->name)+5);

    if(lex->sym->isarray)
    {
        if (!lex->lft)
        {
            info(ERROR, "channel %s is an array.", lex->sym->name);
        }
        if(lex->lft->ntyp != CONST)
        {
            info(ERROR, "idx of chan %s should be CONST\n", lex->sym->name);
        }
        sprintf(vname, "%s_%d", lex->sym->name, lex->lft->val + 1);
    }
    else
    {
        strcpy(vname, lex->sym->name);
    }

    if (!(var = lookupVar(vname)))
    {
        info(ERROR, "variable %s without declared.", vname);
    }
    return var;
}

static void startRunProcess(Lextok *run, int mark)
{
    char *name;
    ProcList *p;
    Module *m, *pre_context;
    Varinf *sv, *var;
    VarList *vl;
    Next *nt;
    Express *e;
    Lextok *arg;

    if(!(p = lookupProcess(run->sym->name)))
    {
        info(ERROR, "proctype %s not found", run->sym->name);
    }
    m = p->m;

    // create variable of process
    name = (char *)emalloc(sizeof(m->name) + 7);
    sprintf(name, "p%d_%s", mark, m->name);
    sv = setVarOfModule(m, name);
    s_context->varl = addVarToVarList(s_context->varl, sv);

    if(m->usglob)
    {
        s_context->usglob = 1;
    }

    // build next to start the run process
    pre_context = s_context;
    s_context = m;
    var = lookupVar("runable");
    s_context = pre_context;
    e = translateConstToExpress(1);
    e->type = ZEROORONE;
    nt = createNext(mark, '#', var, sv, e, 0);
    addNext(nt, s_context);

    // assign actual express to formal parameters
    vl = m->varl;
    for(arg = run->lft; arg; arg = arg->rgt)
    {
        if(!vl || !vl->this)
        {
            info(ERROR, "cannot happen ---- startRunProcess");
        }
        nt = createNext(mark, '#', vl->this, sv, translateExpress(arg->lft), 0);
        addNext(nt, s_context);
        vl = vl->nxt;
    }
}

static void asgnNext(Lextok *lex, Module *sub)
{
    int i;
    Varinf *tvar;
    Next *nt;

    info(DEBUG, "--- --- --- ASGN of %s translating ...", lex->sym->name);
    if(!(tvar = lookupVar(lex->sym->name)))
    {
        info(ERROR, "variable %s without declare", lex->sym->name);
    }

    if(!sameContext(tvar->context,s_context))
    {
        sub->usglob = 1;
    }

    i = isArray(lex->lft);

    nt = createNext(lex->lab, i, tvar, (Varinf *)0, translateExpress(lex->rgt), lex->forecond);
    addNext(nt, sub);
}

static void sendNext(Lextok *lex, Module *sub)
{
    int i;
    Varinf *rvar;
    Module *ch_module;
    VarList *vl;
    Lextok *arg;
    Express *econd;
    Next *nt;

    rvar = chanVar(lex->lft);

    info(DEBUG, "--- --- --- SEND of channel %s translating ...", rvar->name);
    ch_module = rvar->chmod;

    for(vl = ch_module->varl, arg = lex->rgt;
        vl && vl->this->isarray && arg && arg->ntyp == ',';
        vl = vl->nxt, arg = arg->rgt)
    {
        for(i = vl->this->Imin; i <= vl->this->Imax; i++)
        {
            econd = translateBinaryExpress(unless_cond, expressOfPointInChanModule(lex,EQ,i), '&');
            nt = createNext(lex->lab, i, vl->this, rvar, translateExpress(arg->lft), econd);
            addNext(nt, sub);
        }
    }

    while (vl->this->isarray)       // if number of para in send is litter than number of one msg.
    {
        info(WARN,"You miss data of your message which will be replaced by default data");
        vl = vl->nxt;
    }
    if (arg)
    {
        info(WARN,"Your message's data is too long, which will be cut properly");
    }
    econd = translateBinaryExpress(unless_cond, lex->excut_cond, '&');
    nt = createNext(lex->lab, '#', vl->this, rvar, expressOfPointInChanModule(lex,'-',1), econd);
    addNext(nt,sub);
}

static void receiveNext(Lextok *lex, Varinf *rvar, Module *sub)
{
    int i, n = 1, idx;
    Varinf *tvar;
    Module *ch_module;
    VarList *vl;
    Lextok *arg;
    Express *econd, *rval;
    Next *nt;

    info(DEBUG, "--- --- --- RECEIVE of channel %s translating ...", rvar->name);

    ch_module = rvar->chmod;
    econd = translateBinaryExpress(lex->excut_cond, unless_cond, '&');

    for(vl = ch_module->varl, arg = lex->rgt;
        vl && vl->this->isarray;
        vl = vl->nxt)
    {
        for(i = vl->this->Imin; i <= vl->this->Imax; i++)
        {
            if (i == 0)
            {
                rval = (Express *)emalloc(sizeof(Express));
                rval->oper = CONST;
                rval->type = vl->this->type;
                rval->svar = rvar;
                rval->value = vl->this->ini;
            }
            else
            {
                rval = elementOfChanModule(ch_module, n, i-1);
                rval->svar = rvar;
            }
            nt = createNext(lex->lab, i, vl->this, rvar, rval, econd);
            addNext(nt, sub);
        }

        if (arg)
        {
            if(arg->lft->ntyp != NAME || !strcmp(arg->lft->sym->name, "_"))
            {
                arg = arg->rgt;
                n++; continue;
            }

            if(!(tvar = lookupVar(arg->lft->sym->name)))
            {
                info(ERROR, "variable %s without declare", arg->lft->sym->name);
            }

            idx = isArray(arg->lft);
            rval = elementOfChanModule(ch_module, n, i-1);
            rval->svar = rvar;
            nt = createNext(lex->lab, idx, tvar, (Varinf *)0, rval, econd);
            addNext(nt,sub);
            n++;
            arg = arg->rgt;
        }
        else
        {
            info(WARN, "Your accept list of channel is too short, which will make part of msg missing");
        }
   }

   if (arg)
   {
       info(WARN, "Your accept list of channel is too long, whose long part will be ignored");
   }

   nt = createNext(lex->lab, '#', vl->this, rvar, expressOfPointInChanModule(lex,'+',1), econd);
   addNext(nt, sub);
}

static void unlessNext(Lextok *lex, Module *sub)
{
    Sequence *lsq, *rsq;
    Lextok *rlex;
    Element *el, *firstElementOfEscape;

    info(DEBUG, "--- --- --- UNLESS translating ...");

    lsq = lex->sl->this;
    rsq = lex->sl->nxt->this;

    info(DEBUG, "--- --- --- escape clause translating ...");
    for (el = rsq->frst; el; el = el->nxt)
    {
        buildNext(el, sub);
        if (el == rsq->extent)
        {
            break;
        }
    }

    firstElementOfEscape = rsq->frst;
    while(firstElementOfEscape->n->ntyp == '.' || firstElementOfEscape->n->ntyp == 0)
    {
        firstElementOfEscape = firstElementOfEscape->nxt;
    }
    rlex = firstElementOfEscape->n;
    if (rlex->excut_cond != NULL)
    {
        info(DEBUG, "--- --- --- set unless condition and main clause translating ...");

        for (el = lsq->frst; el; el = el->nxt)
        {
            buildNext(el, sub);
            if (el == lsq->extent)
            {
                break;
            }
        }
    }
    else
    {
        info(DEBUG, "--- --- --- escape clause excutable with no condition, ignore main clause.");
    }
}

static void ifOrDoNext(SeqList *sl, Module *sub)
{
    Element *el;

    info(DEBUG, "--- --- --- IF/DO translating ...");

    for(; sl; sl = sl->nxt)
    {
        for(el = sl->this->frst; el; el = el->nxt)
        {
            buildNext(el, sub);
            if (el == sl->this->extent)
            {
                break;
            }
        }
    }
}

static int isExcute(Lextok *lex)
{
    SeqList *sl;
    Element *e;

    for(sl = lex->sl; sl; sl = sl->nxt)
    {
        e = sl->this->frst;
        while(e->n->ntyp == '.' || e->n->ntyp == 0)
        {
            e = e->nxt;
        }
        if (!e->n->excut_cond || e->n->ntyp == ELSE)    // IF/DO clause is excutable with no condition
        {
            return 1;
        }
    }

    return 0;
}

static void atomNext(Sequence *s, Module *sub)
{
    Element *el = s->frst;

    info(DEBUG, "--- --- --- ATOMIC/D_STEP translating ...");

    while (el)
    {
        buildNext(el, sub);
        if(el == s->extent)
        {
            break;
        }
        el = el->nxt;
    }
}

/*
 * when the express returned by this function is TRUE, it means
 * the statement will not escape.
 */
static Express *notEscapeCondition(SeqList *sl)
{
    Element *firstElement;
    SeqList *l;
    Express *e = (Express *)0, *c;

    for(l = sl; l; l = l->nxt)
    {
        firstElement = l->this->frst;
        while(firstElement->n->ntyp == '.' || firstElement->n->ntyp == 0)
        {
            firstElement = firstElement->nxt;
        }

        if(!firstElement->n->excut_cond)
        {
            info(ERROR, "cannot happen -- in notEscapeCondition");
        }

        c = translateUnaryExpress('!', firstElement->n->excut_cond);
        e = translateBinaryExpress(e, c, '&');
    }

    return e;
}

void buildNext(Element *e, Module *sub)
{
    Varinf  *rvar;
    SeqList *opt;
    Element *el;
    Lextok *lex = e->n;
    Express *rcond, *lcond;

    lex->forecond = notEscapeCondition(e->esc);

    switch (lex->ntyp)
    {
        case '.': case GOTO: case BREAK:
        {
            break;
        }
        case ASGN :
        {
            asgnNext(lex, sub);
            break;
        }
        case 'c':
        case ELSE:
        {
            if(lex->lft && lex->lft->ntyp == RUN)
            {
                startRunProcess(lex->lft, lex->lab);
            }
            else
            {
                lex->excut_cond = translateExpress(lex);
            }
            break;
        }
        case 's':
        {
            lex->excut_cond = expressOfPointInChanModule(lex, GE, 0);
            sendNext(lex, sub);
            break;
        }
        case 'r':
        {
            rvar = chanVar(lex->lft);
            lex->excut_cond = conditionOfChanIo(lex, rvar, 'r');              //when cons in receive args
            receiveNext(lex, rvar, sub);
            break;
        }
        case UNLESS :
        {
            unlessNext(lex, sub);

            el = lex->sl->this->frst;
            while(el->n->ntyp == '.' || el->n->ntyp == 0)
            {
                el = el->nxt;
            }
            lcond = el->n->excut_cond;
            el = lex->sl->nxt->this->frst;
            while(el->n->ntyp == '.' || el->n->ntyp == 0)
            {
                el = el->nxt;
            }
            rcond = el->n->excut_cond;
            /* if either is NULL, unless clause is excutable with no condition */
            if (lcond && rcond)
            {
                lex->excut_cond = translateBinaryExpress(lcond, rcond, '|');
            }
            break;
        }
        case IF : case DO :
        {
            ifOrDoNext(lex->sl, sub);

            if (!isExcute(lex))
            {
                for(opt = lex->sl; opt; opt = opt->nxt)
                {
                    el = opt->this->frst;
                    while(el->n->ntyp == '.' || el->n->ntyp == 0)
                    {
                        el = el->nxt;
                    }
                    lex->excut_cond = translateBinaryExpress(el->n->excut_cond, lex->excut_cond, '|');
                }
            }
            break;
        }
        case ATOMIC :
        case D_STEP :
        {
            atomNext(lex->sl->this, sub);
            el = lex->sl->this->frst;
            while(el->n->ntyp == '.' || el->n->ntyp == 0)
            {
                el = el->nxt;
            }
            lex->excut_cond = el->n->excut_cond;
            break;
        }
        case 0:
        {
            info(DEBUG, "type 0 lextok in buildNext");
            break;
        }
        default :
        {
            info(ERROR, "unexpected code %d in buildNext", lex->ntyp);
        }
    }
}

static int ifElementOfArrayHasNext(Varinf *var, int idx)
{
    ModList *ml;
    NextList *nl;
    Next *n;

    if(!var || !var->isarray)
    {
        info(ERROR, "cannot happen ---in checkElementOfArrayHasNext");
    }

    for(ml = Mlist; ml; ml = ml->next)
    {
        for(nl = ml->this->nextl; nl; nl = nl->nxt)
        {
            n = nl->th;
            if(var == n->lvar && idx == n->idx)
            {
                return 1;
            }
        }
    }

    return 0;
}

static void checkVariableHasNext(Varinf *var)
{
    int idx, result;
    Next *n;

    if(!var->isarray)
    {
       if(!var->hasnext)
       {
           info(DEBUG, "variable %s hasn't been referred", var->name);
           n = createNext(0, '#', var, 0, 0, 0);
           addNext(n, var->context);
       }
    }
    else
    {
        for(idx = var->Imin; idx <= var->Imax; idx++)
        {
            result = ifElementOfArrayHasNext(var, idx);
            if(!result)
            {
                info(DEBUG, "variable %s[%d] hasn't been referred", var->name, idx);
                n = createNext(0, idx, var, 0, 0, 0);
                addNext(n, var->context);
            }
        }
    }
}

void checkIfAllVariableHasNext()
{
    ModList *ml;
    VarList *vl;
    Varinf *v;

    for(ml = Mlist; ml; ml = ml->next)
    {
        vl = ml->this->varl;
        for(; vl; vl = vl->nxt)
        {
            v = vl->this;
            if(v->type != PROCESS && strcmp(v->name, "pc"))
            {
                checkVariableHasNext(v);
            }
        }
    }
}

void addDefaultToAllNext(Module *m)
{
    Next *n, *mn, *pn;
    NextList *nl;

    for(nl = m->nextl; nl; nl = nl->nxt)
    {
        mn = nl->th;
        n = createNext(0, mn->idx, mn->lvar, 0, 0, 0);

        for(pn = mn; mn; pn = mn, mn = mn->nxt)
            ;
        pn->nxt = n;
    }
}
