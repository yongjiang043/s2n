#ifndef _VARIABLE_H
#define _VARIABLE_H

void addVarListToModule(Module *, VarList *);
void checkRange(Varinf *, int);
void setType(Varinf *, unsigned short);
void translateVars(Lextok *, Module *);
void translateSeq(Sequence *, Module *);
void createVarOfChanModule(char *, unsigned short, int, Module *);
void initialModules(int, Module *);
void translateProcess(ProcList *, Module *);

Varinf *setVarOfModule(Module *, char *);
VarList *addVarToVarList(VarList *, Varinf *);
Varinf  *createVar(char *, int, unsigned char, int, int);

#endif
