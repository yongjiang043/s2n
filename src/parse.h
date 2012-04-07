/*****sym : sym.h ********/
/* functions for building spin AST */
#ifndef _PARSE_H
#define _PARSE_H

#include "spin.h"

extern  Symbol  *context;
// extern  Ordered *all_names;
extern  Lextok 	*Mtype;
extern  LexList *Gtype;
extern  ProcList *Plist;
extern  SeqList  *cur_s;
extern  DynamicList *topoOrder;

int     hash(char *);
int     isMtype(char *);
int     isProctype(char *);
int     properEnabler(Lextok *n);
void    checkIfKeyword(char *);
void    remSeq();
void    unRemSeq();
void    pushBreak();
void    checkOptions(Lextok *);
void    setMtype(Lextok *);
void    checkVarDeclare(Lextok *, int);
void    setGloberVar(Lextok *);
void    checkProName(Lextok *);
void    addLextokToSeq(Lextok *);
void    addElementToSeq(Element *,Sequence *);
void    openSeq();
void    checkSequence(Sequence *);
void    jumpOutOfDstep(Sequence *);
void    crossDsteps(Lextok *, Lextok *);
void    splitSequence(Lextok *);
void    checkAtomic(Sequence *, int);
void    addRun(Lextok *);
void    initRunMap();
void    topoSortRunMap();
ProcList *lookupProcess(char *);
Sequence *closeSeq(int);
Element  *getElementWithLabel(Lextok *, int);
Symbol   *getLabel(Element *);
Symbol   *lookup(char *);
Symbol   *popBreak();
Element  *colons(Lextok *);
SeqList  *addSeqToSeqlist(Sequence *,SeqList *);
Lextok   *nn(Lextok *, int, Lextok *, Lextok *);
Lextok   *unlessLextok(Lextok *, Lextok *);
ProcList *setProcList(Symbol *, Lextok *, Sequence *, Lextok *);

#endif
