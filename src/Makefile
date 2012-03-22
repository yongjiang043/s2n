all: s2n
s2n: func.o main.o spinlex.o parse.o y.tab.o marking.o module.o variable.o express.o smvsym.o state.o prosmv.o flow.o showflow.o
	gcc $^ -o $@
	rm y.tab.c y.tab.h
func.o: func.c spin.h main.h spinlex.h
	gcc -c func.c
main.o:main.c spin.h func.h parse.h global.h y.tab.h
	gcc -c main.c
spinlex.o:spinlex.c main.h spin.h parse.h y.tab.h
	gcc -c spinlex.c
parse.o: parse.c spin.h main.h spinlex.h func.h y.tab.h
	gcc -c parse.c
y.tab.o:y.tab.c spin.h spinlex.h parse.h
	gcc -c y.tab.c
y.tab.c y.tab.h: spin.y
	yacc -d spin.y
marking.o: marking.c func.h y.tab.h parse.h main.h
	gcc -c marking.c
module.o: module.c
	gcc -c module.c
variable.o: variable.c
	gcc -c variable.c
express.o: express.c
	gcc -c express.c
smvsym.o: smvsym.c
	gcc -c smvsym.c
state.o: state.c
	gcc -c state.c
prosmv.o: prosmv.c
	gcc -c prosmv.c
flow.o: flow.c
	gcc -c flow.c
showflow.o: showflow.c
	gcc -c showflow.c
copy:
	cp -r * ~/recovery
clean:
	rm *.o
