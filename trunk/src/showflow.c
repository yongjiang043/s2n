#include <stdio.h>
#include "smv.h"
#include "spin.h"
#include "y.tab.h"
#include "prosmv.h"
#include "func.h"
#include "express.h"
/*
extern int atom;

static void putSide(int s)
{
    switch (s)
    {
        case ID:
        {
            nprintf(" & next(sup.exclusive) = %d", module_id);
            break;
        }
        case GLOBAL:
        {
            nprintf(" & next(sup.exclusive) = 0");
            break;
        }
        case NOCHANGE:
        {
            nprintf(" & next(sup.exclusive) = sup.exclusive");
            break;
        }
        default:
        {
            break;
        }
    }
}

static void transOfNotProcessModule(Module *mod)
{
    nspace(6,smv);
    nprintf("running");
    if (mod->type == 'M')
    {
        nprintf(" & next(exclusive) = exclusive\n");
    }
    if (mod->type == 'C')
    {
        nprintf(" & next(sup.exclusive) = sup.exclusive\n");
    }
    nspace(4, smv);
    nprintf("| !running\n");
}

static void conditionOfModule(Module *mod)
{
	Express *module_cond = mod->prov;

    nprintf("running & ");

    nprintf("!");
    if (module_cond->oper != '(' && module_cond->lexp && module_cond->rexp)
    {
        nprintf("(");
        putExpress(module_cond, smv);
        nprintf(")");
    }
    else
    {
        printf("3\n");
        putExpress(module_cond, smv);
        printf("4\n");
    }

    nprintf(" & next(pc) = pc");

    nprintf("\n");

    nspace(4,smv);
    nprintf("| running & ");
    putExpress(module_cond, smv);
}*/

static void putLabNode(LabNode *ld)
{
    int i = 1;
    TranArc *arc;

    for(; ld; ld = ld->next)
    {
        arc = ld->frtarc;

        if(!arc)
        {
            continue;
        }

        do
        {
            if (i == 1)
            {
                i = 0;
                nspace(15, smv);
                nprintf("pc = %d",ld->lab);
            }
            else
            {
                nspace(13, smv);
                nprintf("| pc = %d", ld->lab);
            }

            if (arc->cond)
            {
                nprintf(" & ");
                if (arc->cond->priority > 11)
                {
                    nprintf("(");
                    putExpress(arc->cond, smv);
                    nprintf(")");
                }
                else
                {
                    putExpress(arc->cond, smv);
                }
            }
            nprintf(" & next(pc) = %d\n", arc->adjvex);

            info(DEBUG, "--- --- --- flow M%d -> M%d created.", ld->lab, arc->adjvex);
            arc = arc->nxtarc;
        }
        while (arc);
    }
}

static void putScheduleControl(PointOfScheduleChange *p, int tab)
{
    if(!p)
    {
        return;
    }

    nspace(4, smv);
    nprintf("& ( ");
    for(; p; p = p->next)
    {
        if(p->mark != 0)
        {
            nprintf("pc = %d", p->mark);
            if(p->condition)
            {
                nprintf(" & ");
            }
        }
        putExpress(p->condition, smv);

        nprintf("\n");
        nspace(6, smv);
        if(p->next)
        {
            nprintf("| ");
        }
    }

    if(tab)
    {
        nprintf(" -> running )\n");
    }
    else
    {
        nprintf(" -> !running )\n");
    }
}

void putTrans(Module *mod)
{
    if (mod->type != 'P')
    {
        nspace(4, smv);
        nprintf("!running\n");
        return;
    }

    nspace(6,smv);

    nprintf("( running & (\n");

    putLabNode(mod->pctran);

    nspace(13,smv);
    nprintf(")\n");

    nspace(6,smv);
    nprintf("| !running & next(pc) = pc )\n");

    putScheduleControl(mod->atomicPoint, 1);
    putScheduleControl(mod->blockPoint, 0);
    nprintf("\n");
}
