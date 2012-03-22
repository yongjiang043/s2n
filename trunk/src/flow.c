/*Building the control flow with the transfering of pc.
  The control flow is a map which is dedicated by a adjacency list */

#include <stdlib.h>
#include "smv.h"
#include "spin.h"
#include "y.tab.h"
#include "express.h"
#include "smvsym.h"
#include "parse.h"
#include "func.h"
#include "module.h"

static  int max;
static  int sep = 0;

static void flowSequence(Sequence *, Module *);
void showMap();

static TranArc *buildArc(int adj, Express *c)
{
    TranArc *arc = (TranArc *)emalloc(sizeof(TranArc));

    arc->adjvex = adj;
    arc->cond = c;

    return arc;
}

static void addArc(TranArc *tran, int index, Module *m)
{
    TranArc *arc;
    LabNode *lnode;

    for (lnode = m->pctran; lnode; lnode = lnode->next)
    {
        if(lnode->lab == index)
        {
            break;
        }
    }

    if(!(lnode->frtarc))
    {
        lnode->frtarc = tran;
    }
    else
    {
        for(arc = lnode->frtarc; arc->nxtarc; arc = arc->nxtarc)
            ;
        arc->nxtarc = tran;
    }
}

static PointOfScheduleChange *buildPointOfSchedule(int mark, Express *c)
{
   PointOfScheduleChange *am = (PointOfScheduleChange *)emalloc(sizeof(PointOfScheduleChange));

   am->mark = mark;
   am->condition = c;

   return am;
}

static PointOfScheduleChange *addPointOfSchedule(PointOfScheduleChange *am, PointOfScheduleChange *list)
{
   PointOfScheduleChange *a;

   if(!list)
   {
       list = am;
   }
   else
   {
       for(a = list; a->next; a = a->next)
       {
           if(a->mark == am->mark)
           {
              info(ERROR, "redefine exclusive mark --- cannot happen in addPointOfSchedule");
           }
       }
       a->next = am;
   }

   return list;
}

/*
 * Return the mark of the next statement on the excution path
 */
static int nextMark(Element *ele)
{   
    Element *tele;
    int  nxtlab = 0;
     
    tele = ele->nxt;
    if (ele->n->ntyp == GOTO)
    {
        tele = getElementWithLabel(ele->n, 1);
        if (!ele->n->rgt)
        {
            nxtlab = nextMark(tele);
        }
        else
        {
            nxtlab = tele->n->lab;
        }
    }
    else if (tele->n->ntyp == '@')
    {
        nxtlab = max + 1;
    }
    else if (tele->n->ntyp == '.' || tele->n->ntyp == BREAK || tele->n->ntyp == 0)
    {
        nxtlab = nextMark(tele);
    }
    else
    {
        nxtlab = tele->n->lab;
    }

    return nxtlab;
}

static Element *nextElement(Element *ele)
{
    Element *tele;

    if (ele->n->ntyp == '@')
    {
        return (Element *)0;
    }
    if (ele->n->ntyp == GOTO)
    {
        tele = getElementWithLabel(ele->n, 1);
        if (!ele->n->rgt)
        {
            tele = nextElement(tele);
        }
        return tele;
    }

    tele = ele->nxt;
    if (tele->n->ntyp == '.' || tele->n->ntyp == BREAK || tele->n->ntyp == 0)
    {
        tele = nextElement(tele);
    }

    return tele;
}

/*
 * j = 0 return next(sup.exclusive) = i , j = 1 return next(sup.exclusive) = sup.exclusive
 */
/*
Express *nextExclusive(int i, int j)
{
    Module *pre_context = s_context;
    Express *exp = (Express *)emalloc(sizeof(Express));
    Express *lexp = (Express *)emalloc(sizeof(Express));
    Express *lrexp = (Express *)emalloc(sizeof(Express));

    s_context = Mlist->this;
    lrexp->oper = NAME; lrexp->type = INTEGER;
    if(!(lrexp->var = lookupVar(exclusive)))
    {
        info(ERROR, "variable exclusive without declare -- cannot happen");
    }

    lexp->oper = NEXT; lexp->type = INTEGER;
    lexp->rexp = lrexp;

    exp->oper = EQ; exp->type = BOOLEAN;
    exp->lexp = lexp;
    if (j == 0)
    {
        exp->rexp = translateConstToExpress(i);
    }
    else
    {
        exp->rexp = lrexp;
    }

    s_context = pre_context;
    return exp;
}
*/
/*
static int ifNextElementInAtomic(Element *e)
{
    int value = 0;
    Element *ne;

    ne = nextElement(e);
    if (ne)
    {
        value = ne->n->inatom;
    }

    return value;
}

static int setSide(Element *e)
{
    int next_atom;
    int side = 0;

    next_atom = ifNextElementInAtomic(e);
    if (e->n->inatom && next_atom)
    {
        side = ID;
    }

    return side;
}*/

static void unlessJump(Element *ele, Module *m)
{
    TranArc *arc;
    Element *escapeElement;
    Express *pre = (Express *)0, *escapeCondition;
    SeqList *sl;

    for(sl = ele->esc; sl; sl = sl->nxt)
    {
        escapeElement = sl->this->frst;
        while(escapeElement->n->ntyp == '.' || escapeElement->n->ntyp == 0)
        {
            escapeElement = escapeElement->nxt;
        }
        escapeCondition = escapeElement->n->excut_cond;

        if (!escapeCondition)
        {
            arc = buildArc(escapeElement->n->lab, pre);  // escape and execution of escape statement is atomic
            addArc(arc, ele->n->lab, m);
            break;
        }

        arc = buildArc(escapeElement->n->lab, translateBinaryExpress(pre, escapeCondition, '&'));
        addArc(arc, ele->n->lab, m);

        pre = translateBinaryExpress(pre, translateUnaryExpress('!', escapeCondition), '&');
    }
}

static void block(Element *ele, Module *m)
{
    Express *econd;
    PointOfScheduleChange *p;

    if (ele->n->excut_cond)
    {
        econd = translateUnaryExpress('!', ele->n->excut_cond);
        econd = translateBinaryExpress(ele->n->forecond, econd, '&');

        p = buildPointOfSchedule(ele->n->lab, econd);
        m->blockPoint = addPointOfSchedule(p, m->blockPoint);
    }
}

static void elseJump(Element *me, Element *e, Module *m)
{
    Express *cond = (Express *)0;
    SeqList *sl;
    Element  *t;
    TranArc  *arc;

    for(sl = me->sub; sl; sl = sl->nxt)
    {
        t = sl->this->frst;
        if (!t->n->excut_cond)
        {
            return;
        }
    }

    for(sl = me->sub; sl; sl = sl->nxt)
    {
        t = sl->this->frst;
        if (t == e)
        {
            break;
        }
        cond = translateBinaryExpress(cond, t->n->excut_cond, '|');
    }

    cond = translateUnaryExpress('!', cond);
    e->n->excut_cond = cond;
    cond = translateBinaryExpress(cond, sep?(Express *)0:e->n->forecond, '&');
    arc = buildArc(e->n->lab, cond);
    addArc(arc, me->n->lab, m);
}

static void simpleStep(Element *ele, Module *m)
{
    int nxtlab, already = 0;
    TranArc *tarc;
    Express *econd;
    PointOfScheduleChange *am;

    nxtlab = nextMark(ele);

//    side = setSide(ele);

    if (sep)
    {
        am = buildPointOfSchedule(ele->n->lab, 0);
        m->atomicPoint = addPointOfSchedule(am, m->atomicPoint);
        already = 1;

        if(ele->esc && !ele->n->forecond)
        {
            return;
        }
        tarc = buildArc(nxtlab, 0);
        addArc(tarc, ele->n->lab, m);
    }
    else
    {
        if (ele->esc)
        {
            unlessJump(ele, m);

            if (!ele->n->forecond)
            {
                return;
            }
        }

        econd = translateBinaryExpress(ele->n->forecond, ele->n->excut_cond, '&');

        tarc = buildArc(nxtlab, econd);
        addArc(tarc, ele->n->lab, m);

        if(!already && ele->n->inatom)
        {
            am = buildPointOfSchedule(ele->n->lab, econd);
            m->atomicPoint = addPointOfSchedule(am, m->atomicPoint);
        }
        block(ele, m);
    }
}

static void ifStep(Element *ele, Module *m)
{
    int already = 0;
    SeqList  *seql;
    Express *econd;
    TranArc *tarc;
    PointOfScheduleChange *am;

    if (sep)
    {
        am = buildPointOfSchedule(ele->n->lab, 0);
        m->atomicPoint = addPointOfSchedule(am, m->atomicPoint);
        already = 1;
    }

    if (!sep && ele->esc)
    {
        unlessJump(ele, m);
    }

    if (!sep && (!ele->esc || ele->n->forecond))
    {
        block(ele, m);
    }

    if(!already && ele->n->inatom)
    {
        econd = translateBinaryExpress(ele->n->excut_cond, ele->n->forecond, '&');
        am = buildPointOfSchedule(ele->n->lab, econd);
        m->atomicPoint = addPointOfSchedule(am, m->atomicPoint);
    }

    if (!ele->esc || ele->n->forecond)
    {
        for(seql = ele->n->sl; seql; seql = seql->nxt)
        {
            Element *e = seql->this->frst;
            while(e->n->ntyp == '.' || e->n->ntyp == 0)
            {
                e = e->nxt;
            }
            if (e->n->ntyp != ELSE)
            {
                econd = translateBinaryExpress(sep?(Express *)0:ele->n->forecond, e->n->excut_cond, '&');
                tarc = buildArc(e->n->lab, econd);
                addArc(tarc, ele->n->lab, m);
            }
            else
            {
                elseJump(ele, e, m);
            }
        }
    }

    for(seql = ele->n->sl; seql; seql = seql->nxt)
    {
        sep = 1;
        flowSequence(seql->this, m);
    }
}

static void unlessStep(Element *ele, Module *m)
{
    SeqList *sl;
    PointOfScheduleChange *am;
    Element *e;

    if (sep)
    {
        am = buildPointOfSchedule(ele->n->lab, 0);
        m->atomicPoint = addPointOfSchedule(am, m->atomicPoint);
    }

    sep = 0;

    e = ele->n->sl->nxt->this->frst;
    while (e->n->ntyp == '.' || e->n->ntyp == 0)
    {
        e = e->nxt;
    }
    am = buildPointOfSchedule(e->n->lab, 0);
    m->atomicPoint = addPointOfSchedule(am, m->atomicPoint);

    for(sl = ele->n->sl; sl; sl = sl->nxt)
    {
        flowSequence(sl->this, m);
    }
}

static void flowStep(Element *ele, Module *m)
{
    switch(ele->n->ntyp)
    {
        case 'c':
        case 'r':
        case 's':
        case ASGN:
        case GOTO:
        case ELSE:
        {
            simpleStep(ele, m); break;
        }
        case UNLESS:
        {
            unlessStep(ele, m); break;
        }
        case ATOMIC:
        case D_STEP:
        {
            flowSequence(ele->n->sl->this, m); break;
        }
        case IF:
        case DO:
        {
            ifStep(ele, m); break;
        }
        default:
        {
            info(ERROR, "invalid type -- cannot happen in flowStep."); break;
        }
    }
    sep = 0;
}

static void flowSequence(Sequence *seq, Module *m)
{
    Element *ele;
    unsigned short p;

    for(ele = seq->frst; ele && ele->n->ntyp != '@'; ele = ele->nxt)
    {
        p = ele->n->ntyp;

        if (p != TYPE && p!= '.' && p != BREAK && p != 0)
        {
            flowStep(ele, m);
        }

        if (ele == seq->extent)
        {
            break;
        }
    }
}

void flowModule(ProcList *pro, Module *m)
{
    int i;
    Express *e;
    PointOfScheduleChange *p;

    max = pro->max_mark;        // max label which is one greater than last label
    for(i = max; i > 0; i--)
    {
        LabNode *tnode = (LabNode *)emalloc(sizeof(LabNode));
        tnode->lab = i;
        if (!m->pctran)
        {
            m->pctran = tnode;
        }
        else
        {
            tnode->next = m->pctran;
            m->pctran = tnode;
        }
    }

    if(pro->s)
    {
        flowSequence(pro->s, m);
    }
//    showMap(m);

    /* make process stops when it ends */
    p = buildPointOfSchedule(m->pcmax, 0);
    m->blockPoint = addPointOfSchedule(p, m->blockPoint);

    if(m->prov)
    {
        e = translateUnaryExpress('!', m->prov);
        p = buildPointOfSchedule(0, e);
        m->blockPoint = addPointOfSchedule(p, m->blockPoint);
    }
}

void showMap(Module *m)
{
    LabNode *lnode;
    TranArc *arc;

    for (lnode = m->pctran; lnode; lnode = lnode->next)
    {
        printf("%d-> ",lnode->lab);
        for(arc = lnode->frtarc; arc; arc = arc->nxtarc)
        {
            printf("%d ",arc->adjvex);
        }
        printf("\n");
    }
    printf("\n");
}
