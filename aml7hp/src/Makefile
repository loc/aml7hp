
all: turtle eins.ps

eins.ps: eins.tlt turtle
	./turtle < eins.tlt > eins.ps

lex.yy.o:  lex.yy.c turtle_aml7hp.tab.h symtab.h
	gcc -c lex.yy.c

lex.yy.c: turtle_aml7hp.l
	flex turtle_aml7hp.l
	
turtle_aml7hp.tab.h: turtle_aml7hp.y
	bison -d turtle_aml7hp.y
	
turtle_aml7hp.tab.c: turtle_aml7hp.y
	bison -d turtle_aml7hp.y
	
turtle_aml7hp.tab.o: turtle_aml7hp.tab.c symtab.h
	gcc -c turtle_aml7hp.tab.c

turtle: turtle_aml7hp.tab.o lex.yy.o symtab.c
	gcc lex.yy.o turtle_aml7hp.tab.o symtab.c -ll -o turtle

clean:
	rm -f turtle lex.* turtle_aml7hp.tab.* *.o eins.ps

