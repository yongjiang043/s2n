/*produce SMV file from the parse tree*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "main.h"
#include "spin.h"
#include "y.tab.h"
#include "func.h"
#include "module.h"
#include "marking.h"
#include "express.h"

FILE *smv;

extern void putTrans(Module *);

int nprintf(const char *pattern,...)
{
    va_list arg_ptr;

    va_start(arg_ptr, pattern);

    vfprintf(smv, pattern, arg_ptr);

    va_end(arg_ptr);

    return 0;
}

static void parray(Varinf *var)
{
    if(!var || !(var->isarray))
    {
        return;
    }

    nprintf("array %d..%d of ",var->Imin, var->Imax);
}

void putVar(Varinf *var)
{
    if (!var)
    {
        return;
    }

    info(DEBUG, "variable %s declare ...", var->name);

    nspace(4, smv);
    nprintf("%s : ",var->name); parray(var);

    switch(var->type)
    {
        case BOOLEAN :
        {
            nprintf("boolean;"); break;
        }
        case UNSIGNED :
        {
            nprintf("unsigned word[%d];", var->bits); break;
        }
        case SIGNED :
        {
            nprintf("signed word[%d];", var->bits); break;
        }
        case ENUM :
        {
            putEnum(smv); break;
        }
        case PROCESS :
        {
            char *tname = var->chmod->name;
            int usglob = var->chmod->usglob;
            int runable = var->chmod->runable;

            if(s_context->type == 'M')
            {
                if(usglob && !runable)
                {
                    nprintf("process %s(self);", tname);
                }
                if(!usglob && runable)
                {
                    nprintf("process %s(TRUE);", tname);
                }
                if(usglob && runable)
                {
                    nprintf("process %s(self, TRUE);", tname);
                }
                if(!usglob && !runable)
                {
                    nprintf("process %s;", tname);
                }
            }
            else
            {
                if(usglob && !runable)
                {
                    nprintf("process %s(sup);", tname);
                }
                if(!usglob && runable)
                {
                    nprintf("process %s(FALSE);", tname);
                }
                if(usglob && runable)
                {
                    nprintf("process %s(sup, FALSE);", tname);
                }
                if(!usglob && !runable)
                {
                    nprintf("process %s;", tname);
                }
            }

            break;
        }
        case INTEGER :
        {
            fprintf(smv, "%d..%d;", var->Imin, var->Imax); break;
        }
        default :
        {
            info(ERROR, "cannot happen in putVar -- unknow variable type"); break;
        }
    }

    nprintf("\n");
}

static void handle_varlist(VarList *vl, void (*put)(Varinf *))
{
    for(; vl; vl = vl->nxt)
    {
        put(vl->this);
    }
}

static void init(char *name, Varinf *var)
{
    info(DEBUG, "variable %s's init...", name);

    nspace(4,smv);
    nprintf("init(%s) := ", name);
    switch(var->type)
    {
        case BOOLEAN :
        {
            if(strcmp(name, "runable") == 0 && s_context->runable == 1)
            {
                nprintf("tick");
            }
            else if(var->ini == 1)
            {
                nprintf("TRUE");
            }
            else if (var->ini == 0)
            {
                nprintf("FALSE");
            }
            else
            {
                info(ERROR, "value of variable %s is not boolean", var->name);
            }
            break;
        }
        case UNSIGNED :
        {
            nprintf("uwconst(%d, %d)", var->ini, var->bits); break;
        }
        case SIGNED :
        {
            nprintf("swconst(%d, %d)", var->ini, var->bits); break;
        }
        case ENUM :
        {
            nprintf("%s", translateIntToMtype(var->ini)); break;
        }
        case INTEGER :
        {
            nprintf("%d", var->ini);
            break;
        }
        default :
        {
            break;
        }
    }
    nprintf(";\n");
}

void putInit(Varinf *var)
{
    int i;

    if (!var || var->type == PROCESS)
    {
        return;
    }

    if (var->isarray)
    {
        char *name = (char *)emalloc(strlen(var->name)+6);
        for(i = var->Imin; i <= var->Imax; i++)
        {
            sprintf(name, "%s[%d]", var->name, i);
            init(name, var);
        }
    }
    else
    {
        init(var->name, var);
    }
}


static void putNext(Next *n, char *name)
{
    nspace(15, smv);

    if(n->nowpc == 0)
    {
        nprintf("TRUE   : %s;\n", name);
        return;
    }

    nprintf("pc = %d", n->nowpc);

    if(n->cond)
    {
        nprintf(" & ");
        putExpress(n->cond, smv);
    }

    nprintf(" : ");

    if (n->lvar->type == SIGNED && n->rval->oper == CONST && n->rval->type == INTEGER)
    {
        nprintf("swconst(%d, 32)", n->rval->value);
    }
    else
    {
        putExpress(n->rval, smv);
    }
    nprintf(";\n");
}

static void putNextList(Next *n)
{
    char *name;

    name = (char *)complicatingName(n->lvar, n->svar, n->idx);

    info(DEBUG, "state change of variable %s start ...", name);

    nspace(4, smv);

    if(n->nowpc == 0 && !n->nxt)
    {
        nprintf("next(%s) := %s;\n", name, name);
    }
    else
    {
        nprintf("next(%s) := \n", name);

        nspace(12, smv);
        nprintf("case\n");

//        putProvider(name);

        for(; n; n = n->nxt)
        {
            putNext(n, name);
        }

        nspace(12,smv); nprintf("esac;\n");
    }

    info(DEBUG, "state change of variable %s finished.", name);
}

static void scanNextList(NextList  *nlist)
{
    NextList *nl;

    for(nl = nlist; nl; nl = nl->nxt)
    {
        if (!nl->th)
        {
            info(ERROR, "cannot happen in scanNextList -- struct next is null.");
        }

        putNextList(nl->th);
    }
}

static int isAllProcessVariable(VarList *vl)
{
    for(; vl; vl = vl->nxt)
    {
        if(vl->this->type != PROCESS)
        {
            return 0;
        }
    }

    return 1;
}

static void putModule(Module *md)
{
    if (!md)
    {
        info(ERROR, "can not happen in putmodule --- module is null.");
    }

    s_context = md;

    info(DEBUG, "module %s writting start ...", md->name);

    if(md->usglob && !md->runable)
    {
        nprintf("MODULE %s(sup)\n", md->name);
    }
    else if(!md->usglob && md->runable)
    {
        nprintf("MODULE %s(tick)\n", md->name);
    }
    else if(md->usglob && md->runable)
    {
        nprintf("MODULE %s(sup, tick)\n", md->name);
    }
    else
    {
        nprintf("MODULE %s\n", md->name);
    }

    if(md->varl || md->varpc)
    {
        info(DEBUG, " VAR session writting start ...");
        nspace(2, smv);
        nprintf("VAR\n");
        handle_varlist(md->varl, putVar);
        putVar(md->varpc);
        info(DEBUG, "VAR clause finished.");
    }

    if(!isAllProcessVariable(md->varl) || md->nextl)
    {
        info(DEBUG, "ASSIGN session writting start ...");
        nspace(2, smv);
        nprintf("ASSIGN\n");
        handle_varlist(md->varl, putInit);
        putInit(md->varpc);
        scanNextList(md->nextl);
        info(DEBUG, "ASSIGN session finishend.");
    }

    if(md->pctran || md->type != 'P')
    {
        info(DEBUG, "TRANS session writting start ...");
        nspace(2, smv);
        nprintf("TRANS\n");
        putTrans(md);
        info(DEBUG, "TRANS session finished.");
    }
    else
    {
        info(DEBUG, "there is no TRANS session in this module.");
    }
    nprintf("\n");

    info(DEBUG, "module %s wittern finished.", md->name);
}

static void scanModuleList()
{
    ModList *ml;
    Module  *md;

    for(ml = Mlist; ml; ml = ml->next)
    {
        md = ml->this;
        putModule(md);
    }
}

void prosmv()
{
    if (!trgfile)
    {
        trgfile = (char *)emalloc(15);
        strcpy(trgfile, "mysmv.smv");
    }

    if(!(smv = fopen(trgfile, "w")))
    {
        info(ERROR, "cannot open file %s\n", trgfile);
    }

    scanModuleList();

    info(NOTE, "the NuSMV file produced was saved as file %s", trgfile);
}
