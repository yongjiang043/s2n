%{
#include <stdarg.h>
#include "spin.h"
#include "spinlex.h"
#include "parse.h"
#include "func.h"

void yyerror(char *,...);

#define YYDEBUG 0
#define Stop    nn(ZN,'@',ZN,ZN)

static   int     inatom = 0;
static   int     has_ini = 0;
static   SeqList *slt;

%}

%token   ASSERT PRINT PRINTM
%token   C_CODE C_DECL C_EXPR C_STATE C_TRACK
%token   RUN LEN ENABLED EVAL PC_VAL
%token   TYPEDEF MTYPE INLINE LABEL OF
%token   GOTO BREAK ELSE SEMI
%token   IF FI DO OD SEP
%token   ATOMIC NON_ATOMIC D_STEP UNLESS
%token   TIMEOUT NONPROGRESS
%token   ACTIVE PROCTYPE D_PROCTYPE
%token   HIDDEN SHOW ISLOCAL
%token   PRIORITY PROVIDED
%token   FULL EMPTY NFULL NEMPTY
%token   CONST TYPE XU              /* val */
%token   NAME UNAME PNAME INAME     /* sym */
%token   STRING CLAIM TRACE INIT    /* sym */

%right  ASGN
%left   SND O_SND RCV R_RCV  /* SND doubles as boolean negation */
%left   OR
%left   AND
%left   '|'
%left   '^'
%left   '&'
%left   EQ NE
%left   GT LT GE LE
%left   LSHIFT RSHIFT
%left   '+' '-'
%left   '*' '/' '%'
%left   INCR DECR
%right  '~' UMIN NEG
%left   DOT
%%

/** PROMELA Grammar Rules **/

program: units      { yytext[0] = '\0';} 
       ;

units: unit
     | units unit
     ;

unit: proc      /* proctype { }               */
    | init      /* init { }                   */
    | claim     /* claim disabled             */
    | events    /* event disabled             */
    | one_decl  /* variables, chans           */
    | utype     /* user type disabled         */
    | c_fcts    /* embedded C code disabled   */
    | ns        /* inline disabled            */
    | SEMI      /* optional separator         */
    | error     /* error                      */
    ;

proc: inst      /* optional instantiator */
      proctype NAME   
           { 
              checkProName($3);             /*ensure pname not redefined*/
              context = $3->sym;
              has_ini = 0;
           }
      '(' decl ')'   
           {
              if ( has_ini )
              {
                    info(ERROR, "initializer in parameter list");
              }
           }
      Opt_priority
      Opt_enabler
      body
           { 
              ProcList *rl;
              rl = setProcList($3->sym, $6, $11->sq, $10);
              if ($1 != ZN)
              {
                    if ($1->val <= 0)
                    {
                        info(ERROR, "The number of active proctype is invalid");
                    }
                    if ($1->val > 255)
                    {
                        info(ERROR, "Max number of active proctype is 255");
                    }
                    rl->pronum = $1->val;
              }
              context = ZS;
           }
    ;

proctype: PROCTYPE      { $$ = ZN; }
        | D_PROCTYPE    { info(WARN, "d_proctype is same as proctype in s2n!");  $$ = ZN; }
        ;

inst: /* empty */   { $$ = ZN; }
    | ACTIVE        { $$ = nn(ZN,CONST,ZN,ZN); $$->val = 1; }
    | ACTIVE '[' CONST ']' 
           {
              $$ = nn(ZN,CONST,ZN,ZN); $$->val = $3->val;
           }
    | ACTIVE '[' NAME ']' 
           {
              $$ = nn(ZN,CONST,ZN,ZN);
              $$->val = 0;
              if (!$3->sym->type)
              {
                   info(ERROR, "undeclared variable %s", $3->sym->name);
              }
              else if ($3->sym->ini->ntyp != CONST)
              {
                   info(ERROR, "need constant initializer for %s", $3->sym->name);
              }
              else
              {
                   $$->val = $3->sym->ini->val;
              }
           }
    ;

init: INIT        { checkProName($1); context = $1->sym; }
      Opt_priority
      body      
           { 
              ProcList *rl;
              rl = setProcList(context, ZN, $4->sq, ZN);
              rl->pronum = 1;
              context = ZS;
           }
    ;

claim: CLAIM body
            { 
               info(WARN, "never claim disabled and ignored in s2n!");
            }
     ;

events: TRACE body
            { 
               info(WARN, "trace/notrace disabled and ignored in s2n!");
            }
      ;
   
utype: TYPEDEF NAME '{' decl_lst '}'
            { 
               info(WARN, "struct disabled and its definition ignored in s2n!");
            }
     ; 
nm: NAME
  ;

ns: INLINE nm '(' args ')'
         { 
            info(ERROR, "inline not supported in s2n, please use \"#define\"!");
         }
  ;

c_fcts: ccode
      | cstate
      ;

cstate: C_STATE STRING STRING           { info(ERROR, "c_state not allowed in s2n!"); }
      | C_TRACK STRING STRING           { info(ERROR, "c_track not allowed in s2n!"); }
      | C_STATE STRING STRING STRING    { info(ERROR, "c_state not allowed in s2n!"); }
      | C_TRACK STRING STRING STRING    { info(ERROR, "c_track not allowed in s2n!"); }
      ;

ccode: C_CODE   { info(ERROR, "c_code not allowed in s2n!"); }
     | C_DECL   { info(ERROR, "c_decl not allowed in s2n!"); }
     ;

cexpr: C_EXPR   { info(ERROR, "c_expr not allowed in s2n!"); }
     ;

body: '{'               { openSeq(); }
      sequence OS       { addLextokToSeq(Stop); }
      '}'         
            {
               $$->sq = closeSeq(0);
               checkSequence($$->sq);
            }
   ;

sequence: step              
             {
                 if ($1)
                 {
                     if ($1->ntyp == NON_ATOMIC)
                         splitSequence($1);
                     else
                         addLextokToSeq($1);
                 }
             }
        | sequence MS step
             {
                 if ($3)
                 { 
                     if ($3->ntyp == NON_ATOMIC)
                         splitSequence($3);
                     else
                         addLextokToSeq($3); 
                 }
             }
        ;

step: one_decl              { $$ = $1; }
    | XU vref_lst           { info(WARN, "xr/xs disabled and ignored in s2n");}
    | NAME ':' one_decl     { info(ERROR, "label preceding declaration"); }
    | NAME ':' XU           { info(ERROR, "label preceding xr/xs claim"); }
    | stmnt                 { $$ = $1;}
    | stmnt UNLESS stmnt    { $$ = unlessLextok($1, $3); $$->inatom = inatom;}
   ;

vis :
    | HIDDEN
    | SHOW
    | ISLOCAL
    ;

asgn:
    | ASGN
    ;

one_decl: vis TYPE var_list
            { /*
               if(context && $2->val == CHAN)
               {
                   info(ERROR, "declaration of channel can't be local or in parameter list, it should be global");
               }*/
               if ($2->val == MTYPE && !Mtype)
               {
                   info(ERROR, "mtype should be definited before using");
               }
               checkVarDeclare($3, $2->val);
               if (!context) 
               {
                   setGloberVar($3);
               }
               else 
               {
                   $$ = $3;
               }
            }
       | vis TYPE asgn '{' nlst '}' 
            {
               if ($2->val != MTYPE)
               {
                   info(ERROR, "malformed declaration");
               }
               if (context != ZS)
               {
                   info(ERROR, "mtype declaration must be global");
               }
               setMtype($5);
            }
       ;

decl_lst: one_decl          { $$ = nn(ZN, ',', $1, ZN); }
        | one_decl SEMI
          decl_lst          { $$ = nn(ZN, ',', $1, $3); }
        ;

decl: /* empty */       { $$ = ZN; }
    | decl_lst         { $$ = $1; }
    ;

vref_lst: varref
        | varref ',' vref_lst
        ;

var_list: ivar              { $$ = nn($1, TYPE, ZN, ZN); }
        | ivar ',' var_list   { $$ = nn($1, TYPE, ZN, $3); }
        ;

ivar: vardcl              { $$ = $1; }
    | vardcl ASGN expr
           {
              $1->sym->ini = $3; $$ = $1;
              has_ini = 1;
           }
    | vardcl ASGN ch_init
           {
              $1->sym->ini = $3; 
              $1->sym->rev = $3->rev;
              $$ = $1;
              has_ini = 1;
           }
    ;

ch_init: '[' CONST ']' OF '{' typ_list '}'
           {
              $$ = nn(ZN, CHAN, ZN, $6);
              $$->val = $2->val;
           
              if ($2->val < 0)
              {
                   info(ERROR, "capability of channel shouldn't be negative");
              }
              if (0 == $2->val)
              {
                    $$->rev = 1;
                    info(NOTE, "receive of rendezvous can not happen in atomic/d_step");
                    $$->val = 1;
              }
           }
       ;

vardcl: NAME         { $1->sym->nel = 1; $$ = $1; }
      | NAME ':' CONST
          {
              if ($3->val >= 8*sizeof(long))
              {
                    info(WARN, "width-field %s too large, it will be limited as 8*sizeof(long)-1", $1->sym->name);
                    $3->val = 8*sizeof(long)-1;
              }
              $1->sym->nbits = $3->val;
              $1->sym->nel = 1; $$ = $1;
          }
      | NAME '[' CONST ']'
          { 
              if ($3->val < 1)
              {
                    info(WARN, "size of array invalid, and will be replaced by size one");
                    $1->sym->nel = 1;
              }
              else
              {
                    $1->sym->nel = $3->val;
              } 
              $1->sym->isarray = 1;
              $$ = $1; 
          }
      ;

varref: cmpnd                   { $$ = $1; }
      ;

pfld: NAME                      { $$ = nn($1, NAME, ZN, ZN); }
    | NAME  '[' expr ']'        { $$ = nn($1, NAME, $3, ZN); }
    ;

cmpnd: pfld sfld                { $$ = $1; }
     ;

sfld:
    | '.' cmpnd %prec DOT       { info(ERROR, "wrong variable refference : struct disabled in s2n"); }
    ;

stmnt: Special                  { $$ = $1; $1->inatom = inatom;}
     | Stmnt                    { $$ = $1; $1->inatom = inatom;}
     ;

Special: varref RCV rargs
           {
              if($1->sym->type != CHAN)
              {
                   info(ERROR, "an uninitialized chan %s", $1->sym->name);
              }
              $$ = nn($1, 'r', $1, $3);
           }
       | varref SND margs
           { 
             if($1->sym->type != CHAN)
             {
                   info(ERROR, "an uninitialized chan %s", $1->sym->name);
             }
             
             if($1->sym->rev)
             {
                  openSeq();
                  $$ = nn(ZN, ATOMIC, ZN, ZN);
                  Lextok *l = nn($1, 's', $1, $3);
                  l->inatom = 1;
                  addLextokToSeq(l);
                  l = nn(ZN, 'c', nn($1, EMPTY, $1, ZN), ZN);
                  l->inatom = 1;
                  addLextokToSeq(l);
                  $$->sl = addSeqToSeqlist(closeSeq(3), 0);
             }
             else
             {
                  $$ = nn($1, 's', $1, $3);
             }
           }
       | IF options FI
           { 
             $$ = nn(ZN, IF, ZN, ZN);
             $$->sl = $2->sl;
             checkOptions($$);
           }
       | DO                       { pushBreak(); }
         options OD       
           { 
             $$ = nn(ZN, DO, ZN, ZN);
             $$->sl = $3->sl;
             checkOptions($$);
           }
       | BREAK
           { 
             $$ = nn(ZN, GOTO, ZN, ZN);
             $$->sym = popBreak();
           }
       | GOTO NAME
           { 
             $$ = nn($2, GOTO, ZN, $2);
             if ($2->sym->type != 0 &&  $2->sym->type != LABEL)
             {
                    info(ERROR, "bad label-name %s", $2->sym->name);
             }
             $2->sym->type = LABEL;
           }
       | NAME ':' stmnt
           { 
             if ($1->sym->type != 0 &&  $1->sym->type != LABEL)
             {
                    info(ERROR, "bad label-name %s", $1->sym->name);
             }
             $1->sym->type = LABEL;
             $$ = nn($1, ':', ZN, $3);
           }
       ;

Stmnt: varref ASGN full_expr
           {  $$ = nn($1, ASGN, $1, $3);
              // trackvar($1, $3);
              // nochan_manip($1, $3, 0);
              // no_internals($1);
           }
     | varref INCR
           {
              $$ = nn(ZN,CONST, ZN, ZN); $$->val = 1;
              $$ = nn(ZN,  '+', $1, $$);
              $$ = nn($1, ASGN, $1, $$);
              // trackvar($1, $1);
              // no_internals($1);
              if ($1->sym->type == CHAN)
              {
                    info(ERROR, "arithmetic on chan");
              }
           }
     | varref DECR
           { 
              $$ = nn(ZN,CONST, ZN, ZN); $$->val = 1;
              $$ = nn(ZN,  '-', $1, $$);
              $$ = nn($1, ASGN, $1, $$);
              // trackvar($1, $1);
              // no_internals($1);
              if ($1->sym->type == CHAN)
              {
                   info(ERROR, "arithmetic on chan");
              }
           }
     | PRINT '(' STRING prargs ')'  { info(WARN, "print ignored in s2n"); }
     | PRINTM '(' varref ')'        { info(WARN, "printm ignored in s2n"); }
     | PRINTM '(' CONST ')'         { info(WARN, "printm ignored in s2n"); }
     | ASSERT full_expr             { info(WARN, "assert ignored in s2n"); }
     | ccode
/*   | varref R_RCV rargs */
/*   | varref RCV LT rargs GT */
/*   | varref R_RCV LT rargs GT */
/*   | varref O_SND margs */

     | full_expr                    { $$ = nn(ZN, 'c', $1, ZN);    }
     | ELSE                         { $$ = nn(ZN, ELSE, ZN, ZN);   }
     | ATOMIC '{'                   { inatom++; openSeq();        } 
       sequence OS '}'
          { 
               $$ = nn(ZN, ATOMIC, ZN, ZN);
               $$->sl = addSeqToSeqlist(closeSeq(3), 0);
               checkOptions($$);
               checkAtomic($$->sl->this, 0);
               inatom--;
          }
     | D_STEP '{'                   { inatom++; openSeq(); remSeq();    }
       sequence OS '}'
          {
               $$ = nn(ZN, D_STEP, ZN, ZN);
               $$->sl = addSeqToSeqlist(closeSeq(4), 0);
               checkOptions($$);
               checkAtomic($$->sl->this, 1);
               unRemSeq();
               jumpOutOfDstep($$->sl->this);
               inatom--;
           }
     | '{'                          { openSeq(); }
       sequence OS '}'
           {
               $$ = nn(ZN, NON_ATOMIC, ZN, ZN);
               $$->sl = addSeqToSeqlist(closeSeq(5), 0);
               checkOptions($$);
           }
     | NAME '(' args ')' Stmnt       { info(ERROR, "inline not allowed in s2n!"); }
     ;

options: option                     { $$->sl = addSeqToSeqlist($1->sq, 0); }
       | option options             { $$->sl = addSeqToSeqlist($1->sq, $2->sl); }
       ;

option: SEP                         { openSeq(); }
        sequence OS                 { $$->sq = closeSeq(6); }
       ;

OS: /* empty */
  | SEMI                { /* redundant semi at end of sequence */ }
  ;

MS: SEMI                { /* at least one semi-colon */ }
  | MS SEMI             { /* but more are okay too   */ }
  ;

aname: NAME
     | PNAME
     ;

expr: '(' expr ')'      { $$ = nn(ZN, '(', ZN, $2); }
    | expr '+' expr     { $$ = nn(ZN, '+', $1, $3); }
    | expr '-' expr     { $$ = nn(ZN, '-', $1, $3); }
    | expr '*' expr       { $$ = nn(ZN, '*', $1, $3); }
    | expr '/' expr     { $$ = nn(ZN, '/', $1, $3); }
    | expr '%' expr     { $$ = nn(ZN, '%', $1, $3); }
    | expr '&' expr     { $$ = nn(ZN, '&', $1, $3); }
    | expr '^' expr     { $$ = nn(ZN, '^', $1, $3); }
    | expr '|' expr     { $$ = nn(ZN, '|', $1, $3); }
    | expr GT expr      { $$ = nn(ZN,  GT, $1, $3); }
    | expr LT expr      { $$ = nn(ZN,  LT, $1, $3); }
    | expr GE expr      { $$ = nn(ZN,  GE, $1, $3); }
    | expr LE expr      { $$ = nn(ZN,  LE, $1, $3); }
    | expr EQ expr      { $$ = nn(ZN,  EQ, $1, $3); }
    | expr NE expr      { $$ = nn(ZN,  NE, $1, $3); }
    | expr AND expr     { $$ = nn(ZN, AND, $1, $3); }
    | expr OR  expr     { $$ = nn(ZN,  OR, $1, $3); }
    | expr LSHIFT expr  { $$ = nn(ZN, LSHIFT,$1, $3); }
    | expr RSHIFT expr  { $$ = nn(ZN, RSHIFT,$1, $3); }
    | '~' expr          { $$ = nn(ZN, '~', $2, ZN); }
    | '-' expr %prec UMIN   { $$ = nn(ZN, UMIN, $2, ZN); }
    | SND expr %prec NEG    { $$ = nn(ZN, '!', $2, ZN); }

    | '(' expr SEMI expr ':' expr ')'
          {
              $$ = nn(ZN,  OR, $4, $6);
              $$ = nn(ZN, '?', $2, $$);
          }
    | RUN aname         { if(!context)
                          {
                             info(ERROR, "used 'run' outside proctype");
                          }
                        }  
      '(' args ')' 
      Opt_priority      { $$ = nn($2, RUN, $5, ZN); }
    | LEN '(' varref')'
          {
              if($3->sym->type != CHAN)
              {
                  info(ERROR, "an uninitialized chan %s", $3->sym->name);
              }
              $$ = nn($3, LEN, $3, ZN); 
          }
    | ENABLED '(' expr ')'  { info(ERROR, "enabled is not supported yet");}
    | varref RCV '[' rargs ']'
            {
                if($1->sym->type != CHAN)
                {
                     info(ERROR, "an uninitialized chan %s", $1->sym->name);
                }
                $$ = nn($1, 'R', $1, $4);
            }
/*  | varref R_RCV '[' rargs ']'*/
    | varref                 { $$ = $1; }
    | cexpr
    | CONST
         { 
             $$ = nn(ZN,CONST,ZN,ZN);
             $$->ismtyp = $1->ismtyp;
             $$->val = $1->val;
         }
    | TIMEOUT
    | NONPROGRESS
    | PC_VAL '(' expr ')'               /* in never claim which disabled */
    | PNAME '[' expr ']' '@' NAME       /* in never claim which disabled */
    | PNAME '[' expr ']' ':' pfld       /* in never claim which disabled */
    | PNAME '@' NAME                    /* in never claim which disabled */
    | PNAME ':' pfld                    /* in never claim which disabled */
    ;

Opt_priority:
            | PRIORITY CONST            { info(WARN, "priority disabled in s2n!"); $$ = ZN;}
            ;

full_expr: expr              { $$ = $1; }
         | Expr              { $$ = $1; }
         ;

Opt_enabler: /* none */               { $$ = ZN; }
           | PROVIDED '(' full_expr ')'
                {
                    if (!properEnabler($3))
                    {
                          info(WARN, "invalid PROVIDED clause");
                          $$ = ZN;
                    }
                    else
                    {
                          $$ = $3;
                    }
                 }
           | PROVIDED error 
                 { 
                    $$ = ZN;
                    info(ERROR, "usage: provided ( ..expr.. )");
                 }
           ;

       /* an Expr cannot be negated - to protect Probe expressions */
Expr: Probe                      { $$ = $1; }
    | '(' Expr ')'               { $$ = nn(ZN, '(', ZN, $2); }
    | Expr AND Expr              { $$ = nn(ZN, AND, $1, $3); }
    | Expr AND expr              { $$ = nn(ZN, AND, $1, $3); }
    | Expr OR  Expr              { $$ = nn(ZN,  OR, $1, $3); }
    | Expr OR  expr              { $$ = nn(ZN,  OR, $1, $3); }
    | expr AND Expr              { $$ = nn(ZN, AND, $1, $3); }
    | expr OR  Expr              { $$ = nn(ZN,  OR, $1, $3); }
    ;

Probe: FULL '(' varref ')'
             {
                 if($3->sym->type != CHAN)
                 {
                     info(ERROR, "an uninitialized chan %s", $3->sym->name);
                 }
                 else
                 {
                     $$ = nn($3,  FULL, $3, ZN);
                 }
             }
     | NFULL '(' varref ')'
             {
                 if($3->sym->type != CHAN)
                 {
                     info(ERROR, "an uninitialized chan %s", $3->sym->name);
                 }
                 else
                 {
                     $$ = nn($3, FULL, $3, ZN);
                 }
             }
     | EMPTY '(' varref ')'
            {
                 if($3->sym->type != CHAN)
                 {
                     info(ERROR, "an uninitialized chan %s", $3->sym->name);
                 }
                 else
                 {
                     $$ = nn($3, EMPTY, $3, ZN);
                 }
             }
     | NEMPTY '(' varref ')'
             {
                 if($3->sym->type != CHAN)
                 {
                     info(ERROR, "an uninitialized chan %s", $3->sym->name);
                 }
                 else
                 {
                     $$ = nn($3, NEMPTY, $3, ZN);
                 }
             }
     ;

basetype: TYPE
            { 
                 $$->sym = ZS;
                 $$->val = $1->val;
                 if ($$->val == UNSIGNED)
                 { 
                      info(ERROR, "unsigned cannot be used as mesg type");
                 }
                 if ($$->val == CHAN)
                 {
                      info(ERROR, "chan cannot be used as mesg type");
                 }
                 if ($$->val == MTYPE && !Mtype)
                 {
                      info(ERROR, "mtype should be definited before using");
                 }
            }
        | error                     /* e.g., unsigned ':' const */
        ;

typ_list: basetype                      { $$ = nn($1, $1->val, ZN, ZN); }
        | basetype ',' typ_list         { $$ = nn($1, $1->val, ZN, $3); }
        ;

args:                                { $$ = ZN; }                             
    | arg                            { $$ = $1; }
    ;

prargs:
      | ',' arg
      ;

margs: arg                             { $$ = $1; }
     | expr '(' arg ')'                { $$ = nn(ZN, ',', $1, $3); }
     ;

arg: expr                              { $$ = nn(ZN, ',', $1, ZN); }
   | expr ',' arg                      { $$ = nn(ZN, ',', $1, $3); }
   ;

rarg: varref                          { $$ = $1; }
    | EVAL '(' expr ')'               { $$ = nn(ZN,EVAL,$3,ZN);}
    | CONST
          {
              $$ = nn(ZN,CONST,ZN,ZN);
              $$->ismtyp = $1->ismtyp;
              $$->val = $1->val;
          }
    | '-' CONST %prec UMIN       
          {
              $$ = nn(ZN,CONST,ZN,ZN);
              $$->val = - ($2->val);
          }
    ;

rargs: rarg                        { $$ = nn(ZN, ',', $1, ZN); }
     | rarg ',' rargs              { $$ = nn(ZN, ',', $1, $3); }
     | rarg '(' rargs ')'          { $$ = nn(ZN, ',', $1, $3); }
     | '(' rargs ')'               { $$ = $2; }
     ;

nlst: NAME                       
           {
              if(strlen($1->sym->name) == 1)
              {
                 info(WARN, "strlen of mtype constant should be bigger than 1, or NuSMV will not recongize it");
              }
			  checkIfKeyword($1->sym->name);
              $$ = nn($1, NAME, ZN, ZN);
              $$ = nn(ZN, ',', $$, ZN);
           }
    | nlst NAME                    
           {
              if(strlen($2->sym->name) == 1)
              {
                 info(WARN, "strlen of mtype constant should be bigger than 1, or NuSMV will not recongize it");
              }
			  checkIfKeyword($2->sym->name);
              $$ = nn($2, NAME, ZN, ZN);
              $$ = nn(ZN, ',', $$, $1);
           }
    | nlst ','                      { $$ = $1; /* commas optional */ }
    ;
%%

void
yyerror(char *fmt, ...)
{
       info(ERROR, fmt);
}
