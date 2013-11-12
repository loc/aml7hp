/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "hashtable.h"
#include <stack>
#include "errors.h"
        
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this);
  //  root = new Inherit();
}

void Decl::Symtab(Inherit *) {

}

VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

void VarDecl::Symtab(Inherit * root) {
    root->activeScopes->top()->symtab->Enter(this->id->name, this, false);
    this->scope = root->activeScopes->top();
    //printf("%s", this->id->name);
}

void VarDecl::Check(Scope * scope) {
    this->scope->CheckVariableAlreadyDecl(this);
    type->Check(scope);
}
  

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // exte nds can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) {
        extends->SetParent(this);
    }
    else {
        extends = NULL;
    }
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

void ClassDecl::Check(Scope * scope) {
    //parent->classes->Enter(id->name, root);

    this->scope->parent->CheckClassAlreadyDecl(this);

    for (int i=0;i < members->NumElements(); i++) {
        members->Nth(i)->Check(scope);
    }
}

void ClassDecl::Symtab(Inherit * root) {
    Scope * parentScope = root->activeScopes->top();
    Scope * newScope = new Scope();

    this->scope = newScope;

    Inheritable * inheritable = new Inheritable();

    inheritable->scope = newScope;
    inheritable->classDecl = this;

    //deal with implements, extends?
    root->inheritables->Append(inheritable);

    parentScope->symtab->Enter(this->id->name, this, false);
    newScope->parent = parentScope;
    parentScope->children->Append(newScope);

    root->scopes->Append(newScope);
    root->activeScopes->push(newScope);

    for (int i=0;i < members->NumElements(); i++) {
        members->Nth(i)->Symtab(root);
    }

    root->activeScopes->pop();
}



InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::Check(Scope * scope) {
    this->scope->parent->CheckInterfaceAlreadyDecl(this);

    for (int i=0; i < members->NumElements(); i++) {
        members->Nth(i)->Check(scope);
    }
}

void InterfaceDecl::Symtab(Inherit * root) {
    Scope * parentScope = root->activeScopes->top();
    Scope * newScope = new Scope();

    this->scope = newScope;

    parentScope->symtab->Enter(this->id->name, this, false);
    newScope->parent = parentScope;
    parentScope->children->Append(newScope);

    root->scopes->Append(newScope);
    root->activeScopes->push(newScope);

    for (int i=0;i < members->NumElements(); i++) {
        members->Nth(i)->Symtab(root);
    }

    root->activeScopes->pop();
}

	
FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}

void FnDecl::Check(Scope * scope) {

    this->scope->parent->CheckFunctionAlreadyDecl(this);
    this->scope->CheckFunctionOverridesProperly(this);

    for (int i=0; i < formals->NumElements(); i++) {
        formals->Nth(i)->Check(this->scope);
    }
    if (body) {
        body->Check(this->scope);
    }
}

void FnDecl::Symtab(Inherit* root) {
    Scope * parentScope = root->activeScopes->top();

    returnType->Check(parentScope);

    Scope * newScope = new Scope();

    this->scope = newScope;

    parentScope->symtab->Enter(this->id->name, this, false);
    newScope->parent = parentScope;
    parentScope->children->Append(newScope);

    root->scopes->Append(newScope);
    root->activeScopes->push(newScope);

    for (int i=0;i < formals->NumElements(); i++) {
        formals->Nth(i)->Symtab(root);
    }

    if (body) {
        body->Symtab(root);
    }

    root->activeScopes->pop();
}



