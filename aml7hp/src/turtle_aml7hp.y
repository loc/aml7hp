
%{
#include <stdio.h>
#include "symtab.h"
%}

%union { int i; node *n; double d;}

%token GO TURN VAR JUMP
%token FOR STEP TO DO
%token EQUALITY IF THEN ELSE WHILE
%token COPEN CCLOSE
%token SIN COS SQRT
%token <d> FLOAT
%token <n> ID               
%token <i> NUMBER       
%token SEMICOLON PLUS MINUS TIMES DIV OPEN CLOSE ASSIGN
%token GTE LTE UNEQUAL CLOSECURLY OPENCURLY GT LT

%type <n> decl
%type <n> decllist

%%
program: head decllist stmtlist tail;

head: { printf("%%!PS Adobe\n"
               "\n"
	       "newpath\n0 0 moveto\n"
	       );
      };

tail: { printf("closepath\nstroke\n"); };

decllist: ;
decllist: decllist decl;

decl: VAR ID SEMICOLON { printf("/tlt%s 0 def\n",$2->symbol);} ;


stmtlist: ;
stmtlist: stmtlist stmt ;

stmt: ID PLUS PLUS SEMICOLON {printf("tlt%s 1 add /tlt%s exch store\n",$1->symbol, $1->symbol);} ;
stmt: ID MINUS MINUS SEMICOLON {printf("tlt%s 1 sub /tlt%s exch store\n",$1->symbol, $1->symbol);} ;
stmt: ID ASSIGN bool SEMICOLON {printf("/tlt%s exch store\n",$1->symbol);} ;
stmt: GO bool SEMICOLON {printf("0 rlineto\n");};
stmt: JUMP bool SEMICOLON {printf("0 rmoveto\n");};
stmt: TURN bool SEMICOLON {printf("rotate\n");};

stmt: FOR ID ASSIGN bool 
          STEP bool
	  TO bool
	  DO {printf("{ /tlt%s exch store\n",$2->symbol);} 
	     stmt {printf("} for\n");};

stmt: COPEN stmtlist CCLOSE;
stmt: cond;
stmt: WHILE {printf("{ ");} bool {printf("{} {exit} ifelse\n")} OPENCURLY
		stmtlist {printf("} loop\n")} CLOSECURLY;

cond: IF bool THEN codeblock {printf("} ");} ELSE codeblock {printf("} ifelse\n");};
cond: IF bool THEN codeblock {printf("} if\n");};

codeblock: OPENCURLY {printf("{ ");}
			 stmtlist
		   CLOSECURLY;
	
bool: bool EQUALITY expr { printf("eq\n");};
bool: bool UNEQUAL expr { printf("ne\n");};
bool: bool GTE expr { printf("ge\n");};
bool: bool LTE expr { printf("le\n");};
bool: bool GT expr { printf("gt\n");};
bool: bool LT expr { printf("lt\n");};
bool: expr; 

expr: expr PLUS term { printf("add ");};
expr: expr MINUS term { printf("sub ");};
expr: term;

term: term TIMES factor { printf("mul ");};
term: term DIV factor { printf("div ");};
term: factor;

factor: MINUS atomic { printf("neg ");};
factor: PLUS atomic;
factor: SIN factor { printf("sin ");};
factor: COS factor { printf("cos ");};
factor: SQRT factor { printf("sqrt ");};
factor: atomic;



atomic: OPEN bool CLOSE;
atomic: NUMBER {printf("%d ",$1);};
atomic: FLOAT {printf("%f ",$1);};
atomic: ID {printf("tlt%s ", $1->symbol);};


%%
int yyerror(char *msg)
{  fprintf(stderr,"Error: %s\n",msg);
   return 0;
}

int main(void)
{   yyparse();
    return 0;
}

