#include <stdio.h>
#include <stdlib.h>
#include "spin.h"
#include "smv.h"
#include "func.h"
#include "y.tab.h"
#include "smvsym.h"
#include "module.h"
#include "marking.h"

Express *translateExpress(Lextok *);
Express *expressOfPointInChanModule(Lextok *, int, int);
void checkEQandNEExpressionType(Express *);

char *complicatingName(Varinf *v, Varinf *sv, int idx)
{  
    char *name;
    char post[5];

    name = (char *)emalloc((strlen(v->name)+20));

    if(!sameContext(v->context, s_context))
    {
        if(v->context->type == 'M')
        {
            sprintf(name, "sup.%s", v->name);
        }
        else if(sameContext(sv->context, s_context))
        {
            sprintf(name, "%s.%s", sv->name,v->name);
        }
        else
        {
            sprintf(name, "sup.%s.%s", sv->name,v->name);
        }
    }
    else
    {
        sprintf(name, "%s", v->name);
    }

    if (v->isarray && idx != '#')
    {
        sprintf(post, "[%d]", idx);
        strcat(name, post);
    }

    return name;
}

Express *translateConstToExpress(int value)            /* turn const to struct Express */
{
    Express *exp;

    exp = (Express *)emalloc(sizeof(Express));
    exp->oper = CONST;
    exp->type = INTEGER;
    exp->value = value;

    return exp;
}

static void copyExpress(Express *src, Express *trg)
{
    if(!src || !trg)
    {
        return;
    }

    trg->lexp = src->lexp;
    trg->nbit = src->nbit;
    trg->oper = src->oper;
    trg->priority = src->priority;
    trg->rexp = src->rexp;
    trg->svar = src->svar;
    trg->type = src->type;
    trg->value = src->value;
    trg->var = src->var;
}

Express *translateUnaryExpress(int op, Express *rexp)        /*unary operation */
{
    Express *exp;

    exp = (Express *)emalloc(sizeof(Express));
    if(op == '!')
    {
        if(rexp->oper == EQ)
        {
            copyExpress(rexp, exp);
            exp->oper = NE; goto end;
        }
        if(rexp->oper == NE)
        {
            copyExpress(rexp, exp);
            exp->oper = EQ; goto end;
        }
        if(rexp->oper == GT)
        {
            copyExpress(rexp, exp);
            exp->oper = LE; goto end;
        }
        if(rexp->oper == LT)
        {
            copyExpress(rexp, exp);
            exp->oper = GE; goto end;
        }
        if(rexp->oper == LE)
        {
            copyExpress(rexp, exp);
            exp->oper = GT; goto end;
        }
        if(rexp->oper == GE)
        {
            copyExpress(rexp, exp);
            exp->oper = LT; goto end;
        }
        if(rexp->oper == '!')
        {
            free(exp);
            exp = rexp->rexp;
            if(exp->oper == '(')
            {
                exp = exp->rexp;
            }
            goto end;
        }

        exp->priority = 2;

        if (rexp->priority > 2)
        {
            rexp = translateUnaryExpress('(', rexp);
        }
    }
    exp->oper = op;
    exp->type = rexp->type;
    exp->rexp = rexp;

end: return exp;
}

Express *translateBinaryExpress(Express *lexp, Express *rexp, int op)     /* binary boolean operation */
{
    Express *exp;

    if (!lexp)
    {
        if(rexp) rexp->type = BOOLEAN;
        return rexp;
    }
    if (!rexp)
    {
        if(lexp) lexp->type = BOOLEAN;
        return lexp;
    }

    exp = (Express *)emalloc(sizeof(Express));

    exp->oper = op;
    exp->priority = (op == '|')?12:11;

    if (lexp->priority > exp->priority)
    {
        lexp = translateUnaryExpress('(', lexp);
    }
    if (rexp->priority > exp->priority)
    {
        rexp = translateUnaryExpress('(', rexp);
    }
    exp->type = lexp->type = rexp->type = BOOLEAN;
    exp->lexp = lexp;
    exp->rexp = rexp;

    return exp;
}
/*
Express *expressForExclusive(int modid, int bo)    // bo = 0 for "(exclusive != 0 & exclusive != nid)", bo = 1 for "(exclusive = 0 | exclusive = nid)"
{
    Module *pre_context = s_context;
    Express *llexp, *rrexp, *lexp, *rexp;
    Express *exp =(Express *)emalloc(sizeof(Express));

    s_context = Mlist->this;
    exp->oper = '(';

    llexp = (Express *)emalloc(sizeof(Express));
    llexp->oper = NAME; llexp->type = INTEGER;
    if(!(llexp->var = lookupVar(exclusive)))
    {
        info(ERROR, "variable exclusive without declared-- cannot happen");
    }

    rrexp = (Express *)emalloc(sizeof(Express));
    rrexp->oper = CONST; rrexp->type = INTEGER;
    rrexp->value = 0;

    lexp = (Express *)emalloc(sizeof(Express));
    lexp->type = BOOLEAN;
    lexp->priority = 10;
    lexp->lexp = llexp; lexp->rexp = rrexp;

    rrexp = (Express *)emalloc(sizeof(Express));
    rrexp->oper = CONST; rrexp->type = INTEGER;
    rrexp->value = modid;

    rexp = (Express *)emalloc(sizeof(Express));
    rexp->type = BOOLEAN;
    lexp->priority = 10;
    rexp->lexp = llexp; rexp->rexp = rrexp;

    if (bo)
    {
        lexp->oper = rexp->oper = EQ;
        exp->rexp = translateBinaryExpress(lexp, rexp, '|');
        exp->type = exp->rexp->type;
    }
    else
    {
        lexp->oper = rexp->oper = NE;
        exp->rexp = translateBinaryExpress(lexp, rexp, '&');
        exp->type = exp->rexp->type;
    }

    s_context = pre_context;
    return exp;
}*/

/*
 * return element of chan module elej[idx] as an express
 */
Express *elementOfChanModule(Module *ch, int j, int idx)
{      
    int i = 1;
    VarList *v;
    Express *ridx;

    Express *exp = (Express *)emalloc(sizeof(Express));

    exp->oper = NAME;

    for(v = ch->varl; v && v->this->isarray ; v = v->nxt, i++)
    {
        if (i == j)
        {
            break;
        }
    }

    exp->var = v->this;
    exp->type = v->this->type;

    ridx = translateConstToExpress(idx);

    exp->rexp = ridx;

    return exp;
}

/*
 * return the condition express, considering there are consts in args.
 */
Express *conditionOfChanIo(Lextok *lek, Varinf *v, int t)
{
    int i=1, max;
    Lextok *lex, *tlex;
    Express *exp, *texp, *lexp;
    Module *mod = v->chmod;

    if (mod->type != 'C')
    {
        info(ERROR, "MODULE %s is not a chan Module.", mod->name);
    }

    if (t == 's')
    {
        exp = expressOfPointInChanModule(lek, GT, -1);
    }
    else if (t == 'r')
    {
        max = mod->varl->this->Imax;
        exp = expressOfPointInChanModule(lek, LT, max);

        for(lex = lek->rgt; lex && lex->ntyp == ','; lex = lex->rgt)
        {
            if(lex->lft->ntyp == NAME)
            {
                i++;
                continue;
            }

            texp = (Express *)emalloc(sizeof(Express));
            texp->oper = EQ;
            texp->priority = 10;
            texp->type = BOOLEAN;
            texp->rexp = elementOfChanModule(mod, i, max);
            texp->rexp->svar = v;

            if(lex->lft->ntyp == CONST)
            {
                tlex = lex->lft;
            }
            else if(lex->lft->ntyp == EVAL)
            {
                tlex = lex->lft->lft;
            }
            else
            {
                info(ERROR, "wrong syntax in channel send");
            }

            lexp = translateExpress(tlex);
            if (lexp->priority > texp->priority)
            {
                lexp = translateUnaryExpress('(', lexp);
            }
            texp->lexp = lexp;
            checkEQandNEExpressionType(texp);
            i++;
/*
      lexp = (Express *)emalloc(sizeof(Express));
      lexp->oper = '&';  lexp->type = BOOLEAN;
      lexp->lexp = exp; lexp->rexp = texp;
      exp = lexp;*/
            exp = translateBinaryExpress(exp, texp, '&');
        }
    }
    return exp;
}

static void eprintf(char *s, int t)
{
    if(s) printf("\033[;31merror: %s\033[0m", s);
    if(!t) return;

    printf("express usage: Let boolean := bool | bit,\n");
    printf("            signed := byte | pid | short | int | unsigned.\n");
    printf("                      enum := mtype | mtype const\n");
    printf("                   integer := number\n");
    printf("-------------------------------------------------------\n");
    switch (t)
    {
        case 1 :
            printf("+,-,*,/,%%: signed * signed -> signed\n");            // +,-,*,/,mod: signed * signed -> signed
            printf("            signed * integer -> signed\n");           //              signed * signed -> signed
            printf("            integer * signed -> signed\n");           //              signed * signed -> signed
            printf("            integer * integer -> integer\n");         //              signed * signed -> signed
            break;
        case 2 :
            printf("&&, || : boolean * boolean -> boolean\n");            // &,|: boolean * boolean -> boolean
            printf("&, |, ^ : signed * signed -> signed\n");              // &,|,xor: signed * signed -> signed
            printf("          signed * integer -> integer\n");            //          signed * signed -> signed
            printf("          integer * signed -> integer\n");            //          signed * signed -> signed
            printf("          integer * integer -> integer\n");           //          signed * signed -> signed
            break;
        case 3 :
            printf(">, <, >=, <= : signed * signed -> boolean\n");        // >,<,>=,<=: signed * signed -> boolean
            printf("               signed * integer -> boolean\n");       //            signed * signed -> boolean
            printf("               integer * signed -> boolean\n");       //            signed * signed -> boolean
            printf("               integer * integer -> boolean\n");      //            integer * integer -> boolean
            break;
        case 4 :
            printf("==, != : signed * signed -> boolean\n");              // =,!=:  signed * signed -> boolean
            printf("         signed * integer -> boolean\n");             //        signed * signed -> boolean
            printf("         integer * signed -> boolean\n");             //        signed * signed -> boolean
            printf("         integer * integer -> boolean\n");            //        integer * integer -> boolean
            printf("         boolean * boolean -> boolean\n");            //        boolean * boolean -> boolean
            printf("         enum * enum -> boolean\n");                  //        enum * enum -> boolean
            break;
        case 5 :
            printf("<<, >> : signed * integer -> signed\n");              // <<,>> : signed * integer -> signed
            printf("         integer * integer -> signed\n");             // <<,>> : signed * integer -> signed
            break;
        case 6 :
            printf("     ! : boolean -> boolean\n");                      // !:boolean -> boolean
            printf("      ~ : signed -> signed\n");                       //   signed -> signed
            printf("          integer -> integer\n");                     //   signed -> signed
            break;
        case 7 :
            printf("- : signed -> signed\n");                             //-: signed -> signed
            printf("    integer -> integer\n");                           //   integer -> integer
            break;
        case 8 :
            printf("(;:) : boolean * signed * signed -> signed\n");
            printf("       boolean * signed * integer -> signed\n");
            printf("       boolean * integer * signed -> sigend\n");
            printf("       boolean * integer * integer -> integer\n");
            printf("       boolean * boolean * boolean -> boolean\n");
            printf("       boolean * enum * enum -> enum\n");
        default : break;
    }
    printf("-------------------------------------------------------\n");
    allDone(1);
}

/*
 * allowed assignment:
 * = :  boolean <- boolean      |    := : boolean <- boolean
 *      enum    <- enum         |         enum <- enum
 *      integer <- integer      |         integer <- integer
 *      signed  <- signed       |         signed <- signed
 *      signed  <- integer const|         signed <- signed
 */
void checkTypeCompatible(Next *n)
{
    int ltype, rtype;

    ltype = n->lvar->type;
    rtype = n->rval->type;

    if (ltype == BOOLEAN)
    {
        if (rtype == ZEROORONE)
        {
            rtype = n->rval->type = BOOLEAN;
        }
        if (rtype != BOOLEAN)
        {
            info(ERROR, "only boolean can be assigned to boolean\n");
        }
    }
    if (ltype == ENUM && rtype != ENUM)
    {
        info(ERROR, "only enum can be assigned to enum\n");
    }
    if (ltype == INTEGER)
    {
        if (rtype == ZEROORONE)
        {
            rtype = n->rval->type = INTEGER;
        }
        if (rtype != INTEGER)
        {
            info(ERROR, "only integer can be assigned to integer\n");
        }
    }
    if (ltype == SIGNED)
    {
        if (rtype & 0x06)
        {
            rtype = n->rval->type = SIGNED;
        }
        if (rtype != SIGNED)
        {
            info(ERROR, "only signed or integer can be assigned to signed\n");
        }
    }
}

Express *expressOfPointInChanModule(Lextok *le, int op, int rcon)   /* op = 0 for FULL NFULL EMPTY NEMPTY chan express, op != 0 for "point op rcon" */
{
    Varinf *var, *rvar;
    Module *last_context;
    char *cname;
    int size;

    Express *exp = (Express *)emalloc(sizeof(Express));

    exp->lexp = (Express *)emalloc(sizeof(Express));
    exp->rexp = (Express *)emalloc(sizeof(Express));
    exp->lexp->oper = NAME; exp->rexp->oper = CONST;
    exp->lexp->type = exp->rexp->type = INTEGER;

    if(le->lft->lft)
    {
        if(le->lft->lft->ntyp != CONST)
        {
            info(ERROR, "index of chan array should be constant.");
        }
        else
        {
            cname = (char *)emalloc(strlen(le->lft->sym->name) + 5);
            sprintf(cname, "%s_%d", le->lft->sym->name, le->lft->lft->val + 1);
        }
    }
    else
    {
        cname = (char *)emalloc(strlen(le->lft->sym->name) + 1);
        strcpy(cname, le->lft->sym->name);
    }

    if (!(rvar = lookupVar(cname)))
    {
        info(ERROR, "variable %s without declared.", cname);
    }

    last_context = s_context;
    s_context = rvar->chmod;

    if(!(var = lookupVar(point)))
    {
        info(ERROR, "variable point without declared -- cannot happen");
    }

    exp->lexp->svar = rvar;
    exp->lexp->var = var;

    s_context = last_context;
    if(!sameContext(rvar->context, s_context))
    {
        s_context->usglob = 1;
    }

    if (!op)
    {
        size = var->Imax;
        exp->type = BOOLEAN;
        switch(le->ntyp)
        {
            case  NFULL:
            {
                exp->oper = GT ; exp->rexp->value = -1; break;
            }
            case   FULL:
            {
                exp->oper = EQ ; exp->rexp->value = -1; break;
            }
            case NEMPTY:
            {
                exp->oper = LT ; exp->rexp->value = size; break;
            }
            case  EMPTY:
            {
                exp->oper = EQ ; exp->rexp->value = size; break;
            }
        }
        exp->priority = 10;
    }
    else
    {
        exp->oper = op;
        exp->type = (op == '+' || op == '-')?INTEGER:BOOLEAN;
        exp->priority = (op == '+' || op == '-')?6:10;
        exp->rexp->value = rcon;
    }

    return exp;
}

void
checkEQandNEExpressionType(Express *exp)
{
        int tmp = exp->lexp->type | exp->rexp->type;

        if ((tmp & 0x08) && !(tmp & 0xF1))
        {
            exp->lexp->type = exp->rexp->type = SIGNED;
        }
        else if (!(tmp & 0xF9))
        {
            exp->lexp->type = exp->rexp->type = INTEGER;
        }
        else if (!(tmp & 0xFC))
        {
            exp->lexp->type = exp->rexp->type = BOOLEAN;
        }
        if (exp->lexp->type != exp->rexp->type)
        {
            eprintf("Type System Violation detected\n", 4);
        }
        exp->type = BOOLEAN;
}

Express *translateExpress(Lextok *le)
{
    Express *exp = (Express *)emalloc(sizeof(Express));
    int ltype, rtype, tmp;

    switch(le->ntyp)
    {
        case CONST :
        {
             exp->oper = CONST;
             if (le->ismtyp)
             {
                 exp->type = ENUM;
             }
             else if (le->val == 1 || le->val == 0)
             {
                 exp->type = ZEROORONE;         // it may be changed to SIGNED or BOOLEAN later.
             }
             else
             {
                 exp->type = INTEGER;
             }
             exp->value = le->val;
             break;
        }
        case NAME :
        {
            exp->oper = NAME;
            if (!(exp->var = lookupVar(le->sym->name)))
            {
                info(ERROR, "variable %s without declare.", le->sym->name);
            }
            if(exp->var->context->type == 'M')
            {
                s_context->usglob = 1;
            }

            exp->type = exp->var->type;
            if (exp->type == PROCESS)
            {
                info(ERROR, "process type variable can not in expression");
            }
            exp->nbit = exp->var->bits;
            if(le->lft)
            {
                if (le->lft->ntyp != CONST || le->lft->ismtyp)
                {
                    info(ERROR, "line %d: idx of array should be const integer\n", le->ln);
                }
                else
                {
                    exp->rexp = translateExpress(le->lft);
                    exp->rexp->type = INTEGER;
                }
            }
            break;
        }
        case '(' :
        {
            exp->oper = '(';
            exp->rexp = translateExpress(le->rgt);
            exp->type = exp->rexp->type;
            break;
        }
        case '+' : case '-' :
        case '*' : case '/' : case '%' :
        {
            exp->oper = le->ntyp;
            if (le->ntyp == '+'||le->ntyp == '-')
            {
                exp->priority = 6;
            }
            else
            {
                exp->priority = 5;
            }
            exp->lexp = translateExpress(le->lft);
            exp->rexp = translateExpress(le->rgt);
            tmp = exp->lexp->type | exp->rexp->type;
            if (tmp & 0x31)
            {
                eprintf("Type System Violation detected\n", 1);
            }
            if (tmp & 0x08)
            {
                exp->lexp->type = exp->rexp->type = SIGNED;
            }
            else
            {
                exp->lexp->type = exp->rexp->type = INTEGER;
            }
            exp->type = exp->lexp->type;
            break;
        }
        case GT  : case LT : case GE : case LE :
        {
            exp->oper = le->ntyp;
            exp->priority = 10;
            exp->lexp = translateExpress(le->lft);
            exp->rexp = translateExpress(le->rgt);
            tmp = exp->lexp->type | exp->rexp->type;
            if (tmp & 0x31)
            {
                eprintf("Type System Violation detected\n", 3);
            }
            if (tmp & 0x08)
            {
                exp->lexp->type = exp->rexp->type = SIGNED;
            }
            else
            {
                exp->lexp->type = exp->rexp->type = INTEGER;
            }
            exp->type = BOOLEAN;
            break;
        }
        case EQ  : case NE :
        {
            exp->oper = le->ntyp;
            exp->priority = 10;
            exp->lexp = translateExpress(le->lft);
            exp->rexp = translateExpress(le->rgt);
            checkEQandNEExpressionType(exp);
            break;
        }
        case LSHIFT :
        case RSHIFT :
        {
            exp->oper = le->ntyp;
            exp->priority = 7;
            exp->lexp = translateExpress(le->lft);
            exp->rexp = translateExpress(le->rgt);

            if ((exp->rexp->type & 0xF9) || (exp->lexp->type & 0xF1))
            {
                eprintf("Type System Violation detected\n", 5);
            }
            else
            {
                exp->lexp->type = SIGNED;
                exp->rexp->type = INTEGER;
            }
            exp->type = SIGNED;
            break;
        }
        case OR :case AND :
        {
            exp->oper = le->ntyp;
            exp->priority = (le->ntyp == OR)?12:11;
            exp->lexp = translateExpress(le->lft);
            exp->rexp = translateExpress(le->rgt);
            if ((exp->lexp->type | exp->rexp->type) & 0xFC)
            {
                eprintf("Type System Violation detected\n", 2);
            }
            else
            {
                exp->lexp->type = exp->rexp->type = BOOLEAN;
            }
            exp->type = BOOLEAN;
            break;
        }
        case '|' :case '&': case '^':
        {
            exp->oper = le->ntyp;
            exp->priority = (le->ntyp == '&')?11:12;
            exp->lexp = translateExpress(le->lft);
            exp->rexp = translateExpress(le->rgt);
            if ((exp->lexp->type | exp->rexp->type) & 0xF1)
            {
                eprintf("Type System Violation detected\n", 2);
            }
            exp->type = exp->rexp->type = exp->lexp->type = SIGNED;
            break;
        }
        case '~' :
        {
            exp->oper = '~';
            exp->priority = 2;
            exp->rexp = translateExpress(le->lft);
            if (exp->rexp->type & 0xF1)
            {
                eprintf("Type System Violation detected\n", 6);
            }
            exp->type = exp->rexp->type = SIGNED;
            break;
        }
        case '!' :
        {
            exp->oper = '!';
            exp->priority = 2;
            exp->rexp = translateExpress(le->lft);
            if (exp->rexp->type & 0xFC)
            {
                eprintf("Type System Violation detected\n", 6);
            }
            exp->type = exp->rexp->type = BOOLEAN;
            break;
        }
        case UMIN :
        {
            exp->oper = UMIN;
            exp->priority = 4;
            exp->rexp = translateExpress(le->lft);
            tmp = exp->rexp->type;
            if (tmp & 0xF1)
            {
                eprintf("Type System Violation detected\n", 7);
            }
            if (tmp & 0x02)
            {
                exp->type = exp->rexp->type = INTEGER;
            }
            else
            {
                exp->type = exp->rexp->type;
            }
            exp->value = -exp->rexp->value;
            break;
        }
        case '?' :
        {
            exp->oper = '?';
            exp->priority = 13;
            exp->lexp = translateExpress(le->lft);
            exp->rexp = (Express *)emalloc(sizeof(Express));
            exp->rexp->lexp = translateExpress(le->rgt->lft);
            exp->rexp->rexp = translateExpress(le->rgt->rgt);

            ltype = exp->rexp->lexp->type;
            rtype = exp->rexp->rexp->type;
            tmp = ltype | rtype;

            if (exp->lexp->type & 0xFC)
            {
                eprintf("The 1st argument of ( ; : ) should be boolean!\n", 8);
            }
            else
            {
                exp->lexp->type = BOOLEAN;
            }

            if ((tmp & 0x08) && (!(tmp & 0xF1)))
            {
                exp->rexp->lexp->type = exp->rexp->rexp->type = SIGNED;
            }
            else if ((tmp & 0x04) && (!(tmp & 0xF9)))
            {
                exp->rexp->lexp->type = exp->rexp->rexp->type = INTEGER;
            }
            else if ((tmp & 0x01) && (!(tmp & 0xFC)))
            {
                exp->rexp->lexp->type = exp->rexp->rexp->type = BOOLEAN;
            }

            if (exp->rexp->lexp->type & exp->rexp->rexp->type)
            {
                exp->type = exp->rexp->lexp->type;
            }
            else
            {
                eprintf("The 2nd and 3rd argument of ( ; : ) should be compared!\n", 8);
            }
            break;
        }
        case NFULL  : case FULL:
        case NEMPTY : case EMPTY:
        {
            exp = expressOfPointInChanModule(le, 0, 0);
            break;
        }
        case 'c'  :
        {
            exp = translateExpress(le->lft);
            break;
        }
        case ELSE :
        {
            exp->oper = ELSE;
            break;
        }
        default :
        {
            printf("wrong express --- translateExpress\n");
            break;
        }
    }
    return exp;
}

Express *
expressOfProvider(Lextok *prov)     /*turn provide clause to Express*/
{
    Express *exp, *pexp;

    if (!prov)
    {
        return (Express *)0;
    }

    exp = translateExpress(prov);

    if (exp->type == BOOLEAN)
    {
        pexp = exp;
    }
    else if (exp->type == SIGNED || exp->type == INTEGER)
    {
        pexp = (Express *)emalloc(sizeof(Express));
        pexp->oper = EQ;
        pexp->priority = 6;
        pexp->lexp = exp;
        pexp->rexp = (Express *)emalloc(sizeof(Express));
        pexp->rexp->oper = CONST;
        pexp->rexp->type = exp->type == SIGNED?SIGNED:INTEGER;
        pexp->rexp->value = 1;
    }
    else
    {
        eprintf("incorrect provide clause!\n", 0);
    }

    return pexp;
}

void
putExpress(Express *exp, FILE *f)
{
    if(!exp)
    {
        return;
    }

    switch(exp->oper)
    {
        case CONST:
        {
            if(exp->type == ENUM)
            {
                fputs(translateIntToMtype(exp->value),f);
            }
            else if (exp->type == SIGNED)
            {
                fprintf(f,"swconst(%d,32)",exp->value);
            }
            else if (exp->type == INTEGER)
            {
                fprintf(f,"%d",exp->value);
            }
            else if (exp->type == BOOLEAN)
            {
                fprintf(f,"%s",exp->value?"TRUE":"FALSE");
            }
            else
            {
                info(ERROR, "cannot happen in putExpress");
            }
            break;
        }
        case NAME:
        {
            char *cname = complicatingName(exp->var, exp->svar, '#');
            fputs(cname,f);
            if(exp->rexp)
            {
                fputs("[", f); putExpress(exp->rexp, f);
                fputs("]", f);
            }
            free(cname);
            break;
        }
        case NEXT:
        {
            fputs("next(",f); putExpress(exp->rexp, f); fputs(")", f); break;
        }
        case '(' :
        {
            exp->rexp->type = exp->type;
            fputs("(",f); putExpress(exp->rexp,f); fputs(")",f);break;
        }
        case '+' :
        {
            exp->lexp->type = exp->rexp->type = exp->type;
            putExpress(exp->lexp,f); fputs(" + ",f); putExpress(exp->rexp,f); break;
        }
        case '-' :
        {
            exp->lexp->type = exp->rexp->type = exp->type;
            putExpress(exp->lexp,f); fputs(" - ",f); putExpress(exp->rexp,f);break;
        }
        case '*' :
        {
            exp->lexp->type = exp->rexp->type = exp->type;
            putExpress(exp->lexp,f); fputs(" * ",f); putExpress(exp->rexp,f);break;
        }
        case '/' :
        {
            exp->lexp->type = exp->rexp->type = exp->type;
            putExpress(exp->lexp,f); fputs(" / ",f); putExpress(exp->rexp,f);break;
        }
        case '%' :
        {
            exp->lexp->type = exp->rexp->type = exp->type;
            putExpress(exp->lexp,f); fputs(" mod ",f); putExpress(exp->rexp,f);break;
        }
        case '&' :case AND :
        {
            putExpress(exp->lexp,f); fputs(" & ",f); putExpress(exp->rexp,f);break;
        }
        case '|' :case OR :
        {
            putExpress(exp->lexp,f); fputs(" | ",f); putExpress(exp->rexp,f);break;
        }
        case '^' :
        {
            putExpress(exp->lexp,f); fputs(" xor ",f); putExpress(exp->rexp,f);break;
        }
        case LT :
        {
            putExpress(exp->lexp,f); fputs(" < ",f); putExpress(exp->rexp,f);break;
        }
        case GT :
        {
            putExpress(exp->lexp,f); fputs(" > ",f); putExpress(exp->rexp,f);break;
        }
        case LE :
        {
            putExpress(exp->lexp,f); fputs(" <= ",f); putExpress(exp->rexp,f);break;
        }
        case GE :
        {
            putExpress(exp->lexp,f); fputs(" >= ",f); putExpress(exp->rexp,f);break;
        }
        case EQ :
        {
            if(exp->lexp->oper == NAME && exp->lexp->type == BOOLEAN
            && exp->rexp->oper == CONST && exp->rexp->type == BOOLEAN)
            {
                if(exp->rexp->value == 0)
                {
                    fputs("!", f);
                }
                putExpress(exp->lexp, f);
            }
            else
            {
                putExpress(exp->lexp,f); fputs(" = ",f); putExpress(exp->rexp,f);
            }
            break;
        }
        case NE :
        {
            if(exp->lexp->oper == NAME && exp->lexp->type == BOOLEAN
            && exp->rexp->oper == CONST && exp->rexp->type == BOOLEAN)
            {
                if(exp->rexp->value == 1)
                {
                    fputs("!", f);
                }
                putExpress(exp->lexp, f);
            }
            else
            {
                putExpress(exp->lexp,f); fputs(" != ",f); putExpress(exp->rexp,f);
            }
            break;
        }
        case LSHIFT :
        {
            putExpress(exp->lexp,f); fputs(" << ",f); putExpress(exp->rexp,f);break;
        }
        case RSHIFT :
        {
            putExpress(exp->lexp,f); fputs(" >> ",f); putExpress(exp->rexp,f);break;
        }
        case '!' :case '~':
        {
            fputs("!",f); putExpress(exp->rexp,f);
            break;
        }
        case UMIN :
        {
            fputs("-",f);
            exp->rexp->type = exp->type;
            putExpress(exp->rexp,f); break;
        }
        case '?' :
        {
            exp->rexp->lexp->type = exp->rexp->rexp->type = exp->type;
            putExpress(exp->lexp,f); fputs(" ? ",f);
            putExpress(exp->rexp->lexp,f); fputs(" : ",f);
            putExpress(exp->rexp->rexp,f); break;
        }
        default :
        {
            printf("cannot happen -- putExpress\n"); break;
        }
    }
}
