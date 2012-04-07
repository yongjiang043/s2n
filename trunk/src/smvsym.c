/*symtable of SMV and its related functions*/
#include <stdlib.h>
#include <stdio.h>
#include "spin.h"
#include "smv.h"
#include "func.h"
#include "module.h"
#include "parse.h"

static VarList *nametab[Nhash + 1];

int sameContext(Module *a,Module *b)
{ 
    if(!a || !b)
    {
        return 0;
    }

    return !strcmp(a->name, b->name);
}

Module *lookupModule(char *name)
{
    ModList *mlt;

    for(mlt = Mlist; mlt; mlt = mlt->next)
    {
        if(strcmp(mlt->this->name,name) == 0)
        {
            return mlt->this;
        }
    }

    return NULL;
}

Varinf *lookupVar(char *name)
{
    VarList *sl;
    int h = hash(name);

   info(DEBUG, "looking for variable %s in module %s.", name, s_context->name);

    for (sl = nametab[h]; sl; sl = sl->nxt)
    {
        if (strcmp(sl->this->name, name) == 0
            && (sameContext(sl->this->context, s_context) || sl->this->context->type == 'M'))
        {
            return sl->this;
        }
    }

    return NULL;
}

void ready(Varinf *v)
{
    VarList *sl, *stmp;
    int h;

	checkIfKeyword(v->name);

    h = hash(v->name);

    for (sl = nametab[h]; sl; sl = sl->nxt)
    {
        if (strcmp(sl->this->name, v->name) == 0
           && sameContext(v->context,sl->this->context))
        {
            printf("variable %s in module %s redeclaration!", v->name, v->context->name);
            return;
        }
    }

    stmp = (VarList *)emalloc(sizeof(VarList));
    stmp->this = v;

    stmp->nxt = nametab[h];
    nametab[h] = stmp;
}

