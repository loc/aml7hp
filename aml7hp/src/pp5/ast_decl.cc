/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "tac.h"
        
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}

void VarDecl::Emit() {
    this->scope = this->parent->scope;

    Location * loc = generator->GenTempVar(this->scope->parent == NULL);
    ArrayType * arrayType;
    NamedType * namedType;
    if ((arrayType = dynamic_cast<ArrayType *>(this->type))) {
        loc->SetElemType(GetATypeChar(arrayType->elemType));
        loc->SetType("array");
    }
    else if ((namedType = dynamic_cast<NamedType *>(this->type))) {
        loc->SetType(namedType->id->name);
        loc->classInfo = classLookups->Lookup(namedType->id->name);
    }
    else {
        loc->SetType(this->type->typeName);
    }

    this->scope->symtab->Enter(this->id->name, loc);
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

void ClassDecl::Emit() {
    if (this->parent != NULL) {
        this->scope = this->parent->scope;
    }

    ClassLookup * classLookup = classLookups->Lookup(this->id->name);

    for (int i=0; i < members->NumElements(); i++) {
        if (FnDecl * fnDecl = dynamic_cast<FnDecl *>(members->Nth(i))) {
            fnDecl->Emit();
        }
    }

    generator->GenVTable(this->id->name, classLookup->methodNames);
}

void ClassDecl::EnterScope() {
    Program * program;
    if ((program = dynamic_cast<Program *>(this->parent))) {
        program->classDecls->Enter(this->id->name, this);
    }
}

void ClassDecl::BuildLookups() {
    ClassLookup * classLookup = this->CreateClassLookup();
    classLookups->Enter(this->id->name, classLookup);
}

ClassLookup * ClassDecl::CreateClassLookup() {
    ClassLookup * lookup;
    Program * program = dynamic_cast<Program *>(this->parent);

    if (extends) {
        ClassDecl * ext = program->classDecls->Lookup(extends->id->name);
        lookup = ext->CreateClassLookup();
    }
    else {
        lookup = new ClassLookup();
    }
    for (int i=0; i < members->NumElements(); i++) {
        if (VarDecl * varDecl = dynamic_cast<VarDecl*>(members->Nth(i))) {
            if (!lookup->fields->Lookup(varDecl->id->name)) {
                int * val = new int(lookup->fieldCount * generator->VarSize);
                lookup->fields->Enter(varDecl->id->name, val);
                lookup->types->Enter(varDecl->id->name, GetATypeChar(varDecl->type));
                if (strcmp(GetATypeChar(varDecl->type), "array") == 0) {
                    ArrayType * arrayType;
                    if (arrayType = dynamic_cast<ArrayType*>(varDecl->type)) {
                        char * temp = new char[100];
                        sprintf(temp, "%s_elem", varDecl->id->name);
                        lookup->types->Enter(temp, GetATypeChar(arrayType->elemType));
                    }
                }
                lookup->fieldCount++;
            }
        }
        else if (FnDecl * fnDecl = dynamic_cast<FnDecl *>(members->Nth(i))) {
            const int * offset = lookup->methods->Lookup(fnDecl->id->name);
            if (offset == NULL){
                int * val = new int(lookup->methodCount * generator->VarSize);
                lookup->methods->Enter(fnDecl->id->name, val);
                char * temp = new char[100];
                sprintf(temp, "_%s.%s", this->id->name, fnDecl->id->name);
    
                lookup->methodNames->Append(temp);
                lookup->types->Enter(fnDecl->id->name, GetATypeChar(fnDecl->returnType));
                if (strcmp(GetATypeChar(fnDecl->returnType), "array") == 0) {
                    ArrayType * arrayType;
                    if (arrayType = dynamic_cast<ArrayType*>(fnDecl->returnType)) {
                        char * temp = new char[100];
                        sprintf(temp, "%s_elem", fnDecl->id->name);
                        lookup->types->Enter(temp, GetATypeChar(arrayType->elemType));
                    }
                }
                lookup->methodCount++;
            }
            else {
                int index = *(offset) / generator->VarSize;
                lookup->methodNames->RemoveAt(index);
                char * temp = new char[100];
                sprintf(temp, "_%s.%s", this->id->name, fnDecl->id->name);
                lookup->methodNames->InsertAt(temp, index);
            }
        }
    }
    return lookup;
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}


InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

	
FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::BuildLookups() {
    ClassDecl * parentClass = dynamic_cast<ClassDecl *>(this->parent);
    if (!parentClass) {
        ClassLookup * globalInfo = classLookups->Lookup("&global");
        char * type = GetATypeChar(this->returnType);
        if (strcmp(type, "array") == 0) {
            ArrayType * arrayType;
            if (arrayType = dynamic_cast<ArrayType*>(this->returnType)) {
                char * temp = new char[100];
                sprintf(temp, "%s_elem", this->id->name);
                globalInfo->types->Enter(temp, GetATypeChar(arrayType->elemType));
            }
        }
        globalInfo->types->Enter(this->id->name, type);
    }

}

void FnDecl::Emit() {
    Scope * newScope = new Scope();
    this->scope = newScope;

    char * label = this->id->name;
    ClassDecl * parentClass;

    if (parentClass = dynamic_cast<ClassDecl *>(this->parent)) {
        char temp [100];
        sprintf(temp, "_%s.%s", parentClass->id->name, this->id->name);
        label = temp;
    }

    this->scope->parent = this->parent->scope;

    generator->resetLocals();

    generator->GenLabel(label);

    BeginFunc * begin = generator->GenBeginFunc();

    for (int i=0; i < formals->NumElements(); i++) {
        VarDecl * decl = dynamic_cast<VarDecl *>(formals->Nth(i));
        char temp[10];
        Location * loc;
        sprintf(temp, "%s", decl->id->name);
        int offset = generator->OffsetToFirstParam + ((i) * generator->VarSize);
        if (parentClass) {
            offset = offset + 4;
        }
        loc = new Location(fpRelative, offset, temp);
        ArrayType * array = dynamic_cast<ArrayType *>(decl->type);
        NamedType * object = dynamic_cast<NamedType *>(decl->type);
        if (array) {
            loc->SetType("array");
            loc->SetElemType(GetATypeChar(array->elemType));
        }
        else {
            loc->SetType(GetATypeChar(decl->type));
        }
        this->scope->symtab->Enter(decl->id->name, loc);
    }

    if (body) body->Emit();

    begin->SetFrameSize(generator->localCount * generator->VarSize);

    generator->GenEndFunc();
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}



