/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    /* pp3: here is where the semantic analyzer is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, examining all constructs for compliance
     *      with the semantic rules.  Each node can have its own way of
     *      checking itself, which makes for a great use of inheritance
     *      and polymorphism in the node classes.
     */
  

  this->Symtab();
  this->Inheritance();

  for (int i=0;i < decls->NumElements(); i++) {
    decls->Nth(i)->Check(root->activeScopes->top());
  }
}

void Program::Symtab() {

    root = new Inherit(NULL);

    Scope * GlobalScope = new Scope();

    root->activeScopes->push(GlobalScope);

    for (int i=0;i < decls->NumElements(); i++) {
        decls->Nth(i)->Symtab(root);
    }
}

void Program::Inheritance() {
    // Inheritable * element;
    // Scope * extends;
    // Scope * impl;

    // for (int i=0;i < inheritables->NumElements(); i++) {
    //     element = inheritables->Nth(i);
    //     if (element->classDecl->extends) {
    //         extends = element->scope->FindScopeFromNamedType(element->classDecl->extends);
    //         if(!extends) {
    //             //error, couldn't find extends
    //         }
    //     }

    //     for (int j=0;j < element->classDecl->implements->NumElements(); j++) {
    //         impl = element->scope->FindScopeFromNamedType(element->classDecl->implements->Nth(i));
    //         if(!impl) {
    //             //error, couldn't find impl
    //         }
    //     }


    //     element->scope->extends = extends;
    // }
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}


void StmtBlock::Check(Scope * scope) {
    int i;
    for(i=0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Check(scope);
    }
    for(i=0; i < stmts->NumElements(); i++) {
        stmts->Nth(i)->Check(scope);
    }
}



ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

void ConditionalStmt::Check(Scope * scope) {
    test->Check(scope);
    body->Check(scope);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

void LoopStmt::Check(Scope * scope) {
    test->Check(scope);
    body->Check(scope);
}

void ForStmt::Check(Scope * scope) {
    test->Check(scope);
    body->Check(scope);
    init->Check(scope);
    step->Check(scope);
}

void WhileStmt::Check(Scope * scope) {
    test->Check(scope);
    body->Check(scope);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::Check(Scope * scope) {
    test->Check(scope);
    body->Check(scope);
    elseBody->Check(scope);
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}


void ReturnStmt::Check(Scope * scope) {
    expr->Check(scope);
}
  
PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::Check(Scope * scope) {
    for(int i=0; i < args->NumElements(); i++) {
        args->Nth(i)->Check(scope);
    }
}


