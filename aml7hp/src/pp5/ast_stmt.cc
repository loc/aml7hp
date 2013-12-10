/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "errors.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
    classDecls = new Hashtable<ClassDecl *>;
}

void Program::Check() {
    /* You can use your pp3 semantic analysis or leave it out if
     * you want to avoid the clutter.  We won't test pp5 against 
     * semantically-invalid programs.
     */
}
void Program::Emit() {
    /* pp5: here is where the code generation is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, generating instructions as you go.
     *      Each node can have its own way of translating itself,
     *      which makes for a great use of inheritance and
     *      polymorphism in the node classes.
     */
    Scope * newScope = new Scope();
    this->scope = newScope;

    classLookups->Enter("&global", new ClassLookup());

    for(int i=0; i < decls->NumElements(); i++) {
        decls->Nth(i)->EnterScope();
    }

    for(int i=0; i < decls->NumElements(); i++) {
        decls->Nth(i)->BuildLookups();
    }

    if (classLookups->Lookup("&global")->types->Lookup("main") == NULL) {
        return ReportError::NoMainFound();
    }

    for(int i=0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Emit();
    }
    generator->DoFinalCodeGen();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::Emit() {
    this->scope = this->parent->scope;
    for(int i=0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Emit();
    }
    for(int i=0; i < stmts->NumElements(); i++) {
        stmts->Nth(i)->Emit();
    }
}

void Stmt::Emit() {

}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

void ForStmt::Emit() {
    this->scope = this->parent->scope;

    char * loopLabel = generator->NewLabel();
    char * continueLabel = generator->NewLabel();

    this->breakLabel = continueLabel;

    init->Eval();
    generator->GenLabel(loopLabel);
    generator->GenIfZ(test->Eval(), continueLabel);
    body->Emit();
    step->Eval();
    generator->GenGoto(loopLabel);
    generator->GenLabel(continueLabel);

}

void WhileStmt::Emit() {
    this->scope = this->parent->scope;

    char * loopLabel = generator->NewLabel();
    char * continueLabel = generator->NewLabel();  

    this->breakLabel = continueLabel;

    generator->GenLabel(loopLabel);
    Location * cond = test->Eval();
    generator->GenIfZ(cond, continueLabel);
    body->Emit();
    generator->GenGoto(loopLabel);
    generator->GenLabel(continueLabel);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::Emit() {
    this->scope = this->parent->scope;
    Location * cond = test->Eval();
    char * ifZLabel = generator->NewLabel();;
    char * continueLabel = ifZLabel;
    if (elseBody) {
        continueLabel = generator->NewLabel();
    }
    generator->GenIfZ(cond, ifZLabel); 
    body->Emit();
    generator->GenGoto(continueLabel);
    if (elseBody) {
        generator->GenLabel(ifZLabel);
        elseBody->Emit();
    }
    generator->GenLabel(continueLabel);
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}

void ReturnStmt::Emit() {
    return generator->GenReturn(expr->Eval());
}
  
void BreakStmt::Emit() {
    Node * node = this->parent;
    LoopStmt * loopStmt;
    while(node) {
        if (loopStmt = dynamic_cast<LoopStmt*>(node)) {
            generator->GenGoto(loopStmt->breakLabel);
        }
        node = node->parent;
    }
}

PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}
    
void PrintStmt::Emit() {
    Expr * arg;
    Location * loc;
    for (int i=0; i < args->NumElements(); i++) {
        arg = args->Nth(i);
        loc = arg->Eval();
        if (strcmp(loc->GetType(), "int") == 0) {
            generator->GenBuiltInCall(PrintInt, loc);
        }
        else if (strcmp(loc->GetType(), "string") == 0) {
            generator->GenBuiltInCall(PrintString, loc);
        }
        else if (strcmp(loc->GetType(), "bool") == 0) {
            generator->GenBuiltInCall(PrintBool, loc);
        }
        else {
            printf("can't print this type");
        }
    }
}
