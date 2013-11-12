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

    this->scope = GlobalScope;

    root->activeScopes->push(GlobalScope);

    for (int i=0;i < decls->NumElements(); i++) {
        decls->Nth(i)->Symtab(root);
    }
}

void Program::Inheritance() {
    Inheritable * element;
    Scope * extends = NULL;
    Decl * impl;
    Type * newType;
    ClassDecl * classDecl;

    for (int i=0;i < root->inheritables->NumElements(); i++) {
        element = root->inheritables->Nth(i);
        newType = new Type(element->classDecl->GetId()->name);
        if (element->classDecl->extends) {
            classDecl = element->classDecl->scope->FindClassDeclFromNamedType(element->classDecl->extends);
            if (classDecl) {
                extends = classDecl->scope;
            }
            if(!extends) {
                //error, couldn't find extends
                ReportError::IdentifierNotDeclared(element->classDecl->extends->GetId(),LookingForClass);
            }
            if (extends) {
                newType->compatables->Append(classDecl);
            }
            element->classDecl->scope->extends = extends;
        }

        for (int j=0;j < element->classDecl->implements->NumElements(); j++) {
            impl = element->classDecl->scope->FindInterfaceDeclFromNamedType(element->classDecl->implements->Nth(j));
            if(!impl) {
                //error, couldn't find impl
                ReportError::IdentifierNotDeclared(element->classDecl->implements->Nth(j)->GetId(),LookingForInterface);
            }
            else {
                element->classDecl->scope->interfaces->Append(impl->scope);
                newType->compatables->Append(impl);
            }
        }

        element->classDecl->type = newType;
        element->scope->extends = extends;
    }
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}


void StmtBlock::Check(Scope * scope) {
    int i;
    for(i=0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Check(this->scope);
    }
    for(i=0; i < stmts->NumElements(); i++) {
        stmts->Nth(i)->Check(this->scope);
    }
}

void StmtBlock::Symtab(Inherit* root){

    Scope * newScope = new Scope();
    newScope->parent = root->activeScopes->top();
    root->activeScopes->top()->children->Append(newScope);

    root->scopes->Append(newScope);
    root->activeScopes->push(newScope);

    this->scope = newScope;

    int i;
    for(i=0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Symtab(root);
    }
    for(i=0; i < stmts->NumElements(); i++) {
        stmts->Nth(i)->Symtab(root);
    }

    root->activeScopes->pop();
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

void LoopStmt::Symtab(Inherit* root){
    body->Symtab(root);
}


void ForStmt::Check(Scope * scope) {
    test->Check(scope);
    body->Check(scope);
    init->Check(scope);
    step->Check(scope);
}

void ForStmt::Symtab(Inherit* root){
    body->Symtab(root);
}

void WhileStmt::Check(Scope * scope) {
    test->Check(scope);
    body->Check(scope);
}

void WhileStmt::Symtab(Inherit* root){
    body->Symtab(root);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::Check(Scope * scope) {
    test->Check(scope);
    body->Check(scope);
    if (elseBody) {
        elseBody->Check(scope);
    }
}

void IfStmt::Symtab(Inherit* root){
    body->Symtab(root);
    if (elseBody) {
        elseBody->Symtab(root);
    }
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

Case::Case(IntConstant *v, List<Stmt*> *s) {
    Assert(s != NULL);
    value = v;
    if (value) value->SetParent(this);
    (stmts=s)->SetParentAll(this);
}

SwitchStmt::SwitchStmt(Expr *e, List<Case*> *c) {
    Assert(e != NULL && c != NULL);
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
}

