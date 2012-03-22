/**********trans.c : tran1.c*************/
/* responsible for VAR init clause in module and chan module */

#include <stdlib.h>
#include "spin.h"
#include "smv.h"
#include "y.tab.h"
#include "module.h"
#include "func.h"
#include "smvsym.h"
#include "marking.h"
#include "state.h"
#include "express.h"

static int paraVar = 0;

void addVarListToModule(Module *sub, VarList *vl)
{
    VarList *tvl;

    if(!sub->varl)
    {
        sub->varl = vl;
    }
    else
    {
        for(tvl = sub->varl; tvl->nxt; tvl = tvl->nxt)
            ;
        tvl->nxt = vl;
   }
}

VarList *addVarToVarList(VarList *vl, Varinf *v)
{
    VarList *vn;

    if(!vl)
    {
        vl = (VarList *)emalloc(sizeof(VarList));
        vl->this = v;
    }
    else
    {
        for (vn = vl; vn->nxt; vn = vn->nxt)
                ;
        vn->nxt = (VarList *)emalloc(sizeof(VarList));
        vn->nxt->this = v;
    }

    return vl;
}

Varinf *createVar(char *name, int type, unsigned char isarray, int min, int max)
{
    Varinf *vf = (Varinf *)emalloc(sizeof(Varinf));

    vf->name = (char *)emalloc(strlen(name)+1);
    strcpy(vf->name, name);
    vf->type = type;
    vf->isarray = isarray;
    vf->Imin = min;
    vf->Imax = max;

    return vf;
}

static int init(Lextok *exp)
{   
    if(!exp) return 0;

    switch (exp->ntyp)
    {
        case '(' : return init(exp->rgt);
        case '+' : return init(exp->lft) + init(exp->rgt);
        case '-' : return init(exp->lft) - init(exp->rgt);
        case '*' : return init(exp->lft) * init(exp->rgt);
        case '/' : return init(exp->lft) / init(exp->rgt);
        case '%' : return init(exp->lft) % init(exp->rgt);
        case '&' : return init(exp->lft) & init(exp->rgt);
        case '^' : return init(exp->lft) ^ init(exp->rgt);
        case '|' : return init(exp->lft) | init(exp->rgt);
        case GT  : return init(exp->lft) > init(exp->rgt);
        case LT  : return init(exp->lft) < init(exp->rgt);
        case GE  : return init(exp->lft) >= init(exp->rgt);
        case LE  : return init(exp->lft) <= init(exp->rgt);
        case EQ  : return init(exp->lft) == init(exp->rgt);
        case NE  : return init(exp->lft) != init(exp->rgt);
        case AND : return init(exp->lft) && init(exp->rgt);
        case OR  : return init(exp->lft) || init(exp->rgt);
        case LSHIFT : return init(exp->lft) << init(exp->rgt);
        case RSHIFT : return init(exp->lft) >> init(exp->rgt);
        case '~'  : return ~ init(exp->lft);
        case UMIN : return - init(exp->lft);
        case '!'  : return ! init(exp->lft);
        case NAME :
        {
            if (!lookupVar(exp->sym->name))
            {
                info(ERROR, "variable %s without declare.", exp->sym->name);
            }
            return init(exp->sym->ini);
        }
        case CONST : return exp->val;
        case '?' :
        {
            if (init(exp->lft))
            {
                return init(exp->rgt->lft);
            }
            else
            {
                return init(exp->rgt->rgt);
            }
        }
        case CHAN :
        {
            info(ERROR, "chan can not appear when initialization");
        }
        default:
        {
            printf("cannot happen -- init\n"); return 0;
        }
    }
}

void checkRange(Varinf *var, unsigned short type, int value)
{
    switch(type)
    {
        case BIT:
        {
            if (value > 1 || value < 0)
            {
                info(ERROR, "variable %s is beyond range of bool/bit", var->name);
            }
            break;
        }
        case BYTE :
        {
            if (value > 255 || value < 0)
            {
                info(WARN, "variable %s is beyond range of byte/pid", var->name);
            }
            break;
        }
        case SHORT :
        {
            if (value > 32767 || value < -32768)
            {
                info(WARN, "variable %s is beyond range of short", var->name);
            }
            break;
        }
        default:
            break;
    }
}
void setType(Varinf *var, unsigned short sty)
{
    switch(sty)
    {
        case BIT :
        {
            var->type =  BOOLEAN ;var->bits = 1;
            break;
        }
        case BYTE :
        {
            var->type =  SIGNED; var->bits = 32;
            break;
        }
        case SHORT :
        {
            var->type = SIGNED; var->bits = 32;
            break;
        }
        case INT :
        {
            var->type =  SIGNED; var->bits = 32;
            break;
        }
        case UNSIGNED :
        {
            var->type =  SIGNED; var->bits = 32;
            break;
        }
        case MTYPE :
        {
            var->type =  ENUM;  break;
        }
        default :
        {
            printf("cannot happen --setype\n"); break;
        }
    }
}

static Varinf *translateVar(Lextok *l)
{
    int inival, bits;
    Varinf *var;

    info(DEBUG, "--- --- --- translating variable %s ...", l->sym->name);

    var = createVar(l->sym->name, 0, 0, 0, 0);
    var->context = s_context;
    setType(var, l->sym->type);

    if(l->sym->isarray)
    {
        if(paraVar)
        {
            info(ERROR, "array is not allowed in parameter list");
        }

        var->Imin = 0;
        var->Imax = l->sym->nel - 1;
        var->isarray = 1;
    }

    if (!l->sym->ini)
    {
        inival = (var->type == ENUM ? 1 : 0);
    }
    else
    {
        if(paraVar)
        {
            info(ERROR, "initializer in parameter list is not allowed");
        }

        inival = init(l->sym->ini);
    }

    var->ini = inival;
    checkRange(var, l->sym->type, var->ini);

    if (l->sym->type == UNSIGNED)
    {
        bits = l->sym->nbits;
        if (var->ini > power(bits)-1 || var->ini < 0)
        {
            info(WARN, "variable %s is beyond range of unsigned with %d bits", var->name, bits);
        }
    }

    ready(var);
    info(DEBUG, "--- --- --- variable %s translated.", l->sym->name);
    return var;
}

/*
 * tranlate one sentence of declaration (not channel) to varlist in SMV
 */
static VarList *translateVarList(Lextok *vlex)
{
    Lextok *tmp;
    VarList *vl = (VarList *)0;
    Varinf *var;

    for (tmp = vlex; tmp; tmp = tmp->rgt)
    {
        var = translateVar(tmp);

        if (!vl)
        {
            vl = (VarList *)emalloc(sizeof(VarList));
            vl->this = var;
        }
        else
        {
            addVarToVarList(vl, var);
        }
    }

    return vl;
}

Varinf *setVarOfModule(Module *chmod, char *name)            /*create chanmodule variable ,their context always main*/
{
    Varinf *var;

    var = createVar(name, PROCESS, 0, 0, 0);

    var->context = s_context;
    var->chmod = chmod;

    ready(var);
    return var;
}

/*
 * translate channel to module and initialize instances
 */
static VarList *translateChan(Lextok *tchan)
{
    int i;
    char *tname;
    Varinf  *var;
    Module  *chmod;
    VarList *vlist = (VarList *)0;

    info(DEBUG, "--- --- --- translating chan %s", tchan->sym->name);
    chmod = lookupChanModule(tchan->sym->ini);    /* build MODULE of chan */

    if(tchan->sym->isarray)                         /* if chan q[3], add variable q_1,q_2,q_3 to module main */
    {
        for(i = 1; i <= tchan->sym->nel; i++)
        {
            tname = (char *)emalloc(strlen(tchan->sym->name)+8);
            sprintf(tname,"%s_%d", tchan->sym->name, i);

            info(DEBUG, "--- --- --- add variable %s to varlist.", tname);

            var = setVarOfModule(chmod, tname);
            vlist = addVarToVarList(vlist, var);

            free(tname);
        }
    }
    else
    {
        info(DEBUG, "--- --- --- add variable %s to varlist.", tchan->sym->name);
        /*if chan q, add variable q to module main*/
        var = setVarOfModule(chmod, tchan->sym->name);
        vlist = addVarToVarList(vlist, var);
    }

    info(DEBUG, "--- --- --- chan %s translated.", tchan->sym->name);
    return vlist;
}

static VarList *translateChans(Lextok *vlex)
{
    Lextok  *tchan;
    VarList *vl, *l, *vlist = (VarList *)0;

    for(tchan = vlex; tchan; tchan = tchan->rgt)
    {
        if(!tchan->sym->ini)                             /*chan var should initialize*/
        {
            info(ERROR, "chan %s without initializer!", tchan->sym->name);
        }

        vl = translateChan(tchan);

        if (!vlist)
        {
            vlist = vl;
        }
        else
        {
            for (l = vlist; l->nxt; l = l->nxt)
                ;
            l->nxt = vl;
        }
    }

   return vlist;
}

void translateVars(Lextok *v, Module *m)
{
    VarList *lt;

    if (v->sym->type == CHAN)
    {
        if(paraVar)
        {
            info(ERROR, "channel is not allowed in parameter list");
        }

        lt = translateChans(v);
    }
    else
    {
        lt = translateVarList(v);
    }

    addVarListToModule(m, lt);
}

void translateSeq(Sequence *s, Module *m)
{
    Element *et;
    Varinf *rable;
    Express *e;

    if(m->runable)
    {
        rable = createVar("runable", BOOLEAN, 0, 0, 0);
        rable->ini = 1;
        rable->context = m;
        ready(rable);

        m->varl = addVarToVarList(m->varl, rable);

        e = (Express *)emalloc(sizeof(Express));
        e->var = rable;
        e->oper = NAME;
        e->type = BOOLEAN;

        m->prov = translateBinaryExpress(m->prov, e, '&');
    }

    for(et = s->frst; et && et->n->ntyp != '@'; et = et->nxt)
    {
        if (et->n->ntyp == TYPE)                /* build local varibles */
        {
            translateVars(et->n, m);
        }
        else
        {
            buildNext(et, m);
        }
    }
}

static void translateParameter(Lextok *formalPara, Module *m)
{
    Lextok *fl = formalPara;

    for(; fl; fl = fl->rgt)
    {
        paraVar = 1;
        translateVars(fl->lft, m);
        paraVar = 0;
    }
}

void translateProcess(ProcList *p, Module *m)
{
    if(p->p)
    {
        info(DEBUG, "--- --- --- translating parameter variables ...");
        translateParameter(p->p, m);
        info(DEBUG, "--- --- --- translated");
    }

    translateSeq(p->s, m);
}

/*
 * create variables in module translated from channel
 */
void createVarOfChanModule(char *name, unsigned short type, int size, Module *m)
{
    Varinf *v;
    VarList *vl;

    if (strcmp(name, "point"))
    {
        v = createVar(name, 0, 1, 0, size-1);
        setType(v, type);
        v->ini = (type == MTYPE ? 1 : 0);
    }
    else
    {
        v = createVar("point", type, 0, -1, size - 1);
        v->ini = size - 1;
    }

    v->context = s_context;
    ready(v);

    vl = (VarList *)emalloc(sizeof(VarList));
    vl->this = v;
    addVarListToModule(m, vl);
}

/*
 * initilize instance of module with name.
 */
static void initialModule(char *name, Module *m)
{
    Varinf  *var;
    VarList *lt;

    info(DEBUG, "--- --- create instance %s of module %s ...", name, m->name);
    var = createVar(name, PROCESS, 0, 0, 0);
    var->context = Mlist->this;
    var->chmod = m;
    ready(var);

    lt = (VarList *)emalloc(sizeof(VarList));
    lt->this = var;
    addVarListToModule(Mlist->this, lt);

    info(DEBUG, "--- --- instance %s of module %s created.", name, m->name);
}

/*
 * initilize num of instances of module.
 */
void initialModules(int num, Module *m)
{
    int i;
    char *name;

    if (num > 255)
    {
        info(WARN, "too many process");
    }

    if (num == 1)
    {
        name = (char *)emalloc(strlen(m->name) + 9);
        sprintf(name,"p_%s", m->name);
        initialModule(name, m);
        free(name);
    }
    else
    {
        for (i = 1; i <= num; i++)
        {
            name = (char *)emalloc(strlen(m->name) + 11);
            sprintf(name,"p%d_%s", i, m->name);
            initialModule(name, m);
            free(name);
        }
    }
}
