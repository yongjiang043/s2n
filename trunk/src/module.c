#include <stdlib.h>
#include "spin.h"
#include "smv.h"
#include "main.h"
#include "func.h"
#include "parse.h"
#include "smvsym.h"
#include "variable.h"
#include "marking.h"
#include "express.h"
#include "state.h"
#include "flow.h"

char *point = "point";

Module  *s_context = (Module *)0;     // the current module
ModList *Mlist = (ModList *)0;        // list of all modules

static LexList *chan_list = (LexList *)0;

static Module *createModule(char *name, int type)
{
    Module *m;

    m = (Module *)emalloc(sizeof(Module));

    if(strlen(name) == 1)
    {
        m->name = (char *)emalloc(strlen(name) + 3);
        sprintf(m->name, "%s%s", name, name);
    }
    else
    {
       m->name = (char *)emalloc(strlen(name) + 1);
       strcpy(m->name, name);
    }
    m->type = type;

    return m;
}

static ModList *addModuleToModuleList(ModList *mlist, Module *m)
{
    ModList *tl,*tlt;

    tl = (ModList *)emalloc(sizeof(ModList));
    tl->this = m;

    if(!mlist)
    {
        mlist = tl;
    }
    else
    {
        for(tlt = mlist; tlt->next; tlt = tlt->next)
            ;
        tlt->next = tl;
    }

    return mlist;
}

static void buildMainModule()
{
    LexList *lt;
    Module  *mn;

    info(DEBUG, "--- --- creating module main ...");

    mn = createModule("main", 'M');
    Mlist = addModuleToModuleList(Mlist, mn);

    s_context = mn;
    /*varibles in MOUDLE main and their initialization*/
    info(DEBUG, "--- --- translating global variable to module main ...");

    for (lt = Gtype; lt; lt = lt->next)
    {
        translateVars(lt->this, mn);
    }

    info(DEBUG, "--- --- global variables translated.");

    info(DEBUG, "--- --- module main created.");
}

/*
 * build module of proctype.
 */
static void buildModule(ProcList *pt)
{
    char *tname;
    int n = 1;
    Module  *mn;
    Varinf *pc;

    tname = (char *)emalloc(sizeof(strlen(pt->n->name)+5));
//    strcpy(tname, "P_");                      /* in case strlen(pt->n->name) = 1 */
    strcat(tname, pt->n->name);

    info(DEBUG, "--- --- create module %s ...", tname);
    mn = createModule(tname, 'P');
    mn->runable = pt->runable;

    mn->pcmax = pt->max_mark + 1;
    mn->prov = expressOfProvider(pt->prov);             /* set global constraint on process execution */

    s_context = mn;
    pt->m = mn;

    Mlist = addModuleToModuleList(Mlist, mn);

    info(DEBUG, "--- --- begin translating sequence in proctype %s ...", pt->n->name);
    translateProcess(pt, mn);
    info(DEBUG, "--- --- sequence translated.");

    info(DEBUG, "--- --- add variable pc to varpc ...");
    pc = createVar("pc", INTEGER, 0, 1, pt->max_mark + 1);
    pc->ini = 1;
    pc->context = s_context;
    ready(pc);

    mn->varpc = pc;

    info(DEBUG, "--- --- building constraint on variable pc ...");
    flowModule(pt, mn);
    info(DEBUG, "--- --- constraint in TRANS finished.");

    info(DEBUG, "--- --- module %s created.", mn->name);

    if(pt->pronum > 0)
    {
        initialModules(pt->pronum, mn);           /* build module initializer in main MODULE */
    }
}

static void buildModules()
{
    ProcList *pt;
    DynamicList *dl;

    for (dl = topoOrder; dl; dl = dl->nxtTopoEle)
    {
        pt = dl->host;
        info(DEBUG, "--- proctype %s to module %s start building ...", pt->n->name, pt->n->name);

        buildModule(pt);

        info(DEBUG, "--- proctype %s to module %s builded\n", pt->n->name, pt->n->name);
    }

    s_context = Mlist->this;
}

/*
 * create module name Ch_n_type1_type2_..._typek for [n] of {type1, type2, ..., typek}
 */
static char *createChanName(Lextok *chan)
{
    Lextok *lex;
    char *name = (char *)emalloc(100);

    sprintf(name, "Ch_%d", chan->val);
    for(lex = chan->rgt; lex; lex = lex->rgt)
    {
        const char *typename = translateIntToType(lex->ntyp);

        if (strlen(name) + strlen(typename) > 99)
        {
            info(ERROR, "name of your chan module is too long.");
        }

        strcat(name, "_");
        strcat(name, typename);
    }

    return name;
}
/*
 * build module of channel [size]of {type1, type2, ..., typen}
 * Module chan_%d
 * VAR
 *  ele1 : array 0..size-1 of type1;
 *  ele2 : array 0..size-1 of type2;
 *  ...
 *  elen : array 0..size-1 of typen;
 *  point : -1..size-1
 */
static Module *buildChanModule(Lextok *lt)
{
    Lextok   *lek;
    Module   *mod, *last_context;
    int     size, n;
    char    *pname;

    pname = createChanName(lt);

    info(DEBUG, "--- --- --- --- creating module %s and her variables ...", pname);
    mod = createModule(pname, 'C');

    last_context = s_context;
    s_context = mod;

    size = lt->val;

    for(lek = lt->rgt, n = 1; lek; lek = lek->rgt, n++)
    {
        sprintf(pname, "ele%d", n);
        createVarOfChanModule(pname, lek->ntyp, size, mod);
    }

    createVarOfChanModule("point", INTEGER, size, mod);

    info(DEBUG, "--- --- --- --- module %s created.", mod->name);
    s_context = last_context;
    return mod;
}

/*
 * compare two channel to see whether they are the same type
 * return 0 if they are, else return 1
 */
static int isSameChan(LexList *l, Lextok *c)
{
    Lextok  *t;

    t = l->this;
    if (t->val != c->val) return 1;

    for(; t && c; t = t->rgt, c = c->rgt)
    {
        if (t->ntyp != c->ntyp)
        {
            return 1;
        }
    }

    return c || t ? 1 : 0;
}

static LexList *lookupChan(Lextok *c)
{
    LexList *l1, *l2;

    if(c->ntyp != CHAN)
    {
        info(ERROR, "cannot happen -- channel module is not of chan type!");
    }

    for (l1 = l2 = chan_list; l2; l1 = l2,l2 = l2->next)
    {
        if (!isSameChan(l2, c))            /* if the same chan has existed*/
        {
            return l2;
        }
    }

    l2 = (LexList *)emalloc(sizeof(LexList));
    l2->this = c;

    if (!chan_list)
    {
        chan_list = l2;
    }
    else
    {
        l1->next = l2;
    }

    return l2;
}

/*
 * lookup module of the chan.
 * if the module doesn't exist, create it.
 */
Module *lookupChanModule(Lextok *clex)
{
    LexList *t;
    Module  *m;

    t = lookupChan(clex);

    if (!t->cmod)
    {
        info(DEBUG, "--- --- --- --- module of chan doesnot exists.");
        m = buildChanModule(clex);
        t->cmod = m;
        Mlist = addModuleToModuleList(Mlist, m);
    }
    else
    {
        info(DEBUG, "--- --- --- --- module of chan has already existed.");
        m = t->cmod;
    }

    return m;
}

void build()
{
    ModList *l;

    info(DEBUG, "--- module main start building ...");
    buildMainModule();
    info(DEBUG, "--- module main builded\n");

    buildModules();

    l = Mlist;
    while (l)
    {
        Module *m = l->this;

        addDefaultToAllNext(m);
        l = l->next;
    }

    checkIfAllVariableHasNext();
}
