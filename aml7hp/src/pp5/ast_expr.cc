/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>
#include "errors.h"


IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}

Location * IntConstant::Eval() {
    Location * loc = generator->GenLoadConstant(this->value);
    loc->SetType("int");
    return loc;
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

Location * BoolConstant::Eval() {
    int zeroOrOne = 1;
    if (!this->value) {
        zeroOrOne = 0;
    }
    Location * loc = generator->GenLoadConstant(zeroOrOne);
    loc->SetType("bool");
    return loc;
}


StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Location * StringConstant::Eval() {
    Location * loc = generator->GenLoadConstant(this->value);
    loc->SetType("string");
    return loc;
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}

Location * ArithmeticExpr::Eval() {
    Location * loc;
    if (left == NULL && strcmp(op->tokenString, "-") == 0) {
        loc = generator->GenBinaryOp(op->tokenString, generator->GenLoadConstant(0), right->Eval());
    }
    else {
        loc = generator->GenBinaryOp(op->tokenString, left->Eval(), right->Eval());
    }
    loc->SetType("int");
    return loc;
}

Location * RelationalExpr::Eval() {
    Location * loc;
    if (strcmp(op->tokenString, ">=") == 0) {
        loc = generator->GenBinaryOp("||", generator->GenBinaryOp("<", right->Eval(), left->Eval()), generator->GenBinaryOp("==", left->Eval(), right->Eval()));
    }
    else if (strcmp(op->tokenString, "<=") == 0) {
        loc = generator->GenBinaryOp("||", generator->GenBinaryOp("<", left->Eval(), right->Eval()), generator->GenBinaryOp("==", left->Eval(), right->Eval()));
    }
    else if (strcmp(op->tokenString, ">") == 0) {
        loc = generator->GenBinaryOp("<", right->Eval(), left->Eval());
    }
    else {
        loc = generator->GenBinaryOp(op->tokenString, left->Eval(), right->Eval());
    }
    loc->SetType("bool");
    return loc;
} 

Location * EqualityExpr::Eval() {
    Location * loc;
    if (strcmp(op->tokenString, "!=") == 0) {
        loc = generator->GenBinaryOp("||", generator->GenBinaryOp("<", right->Eval(), left->Eval()), generator->GenBinaryOp("<", left->Eval(), right->Eval()));
    }
    else {
        Location * l = left->Eval();
        Location * r = right->Eval();
        if ((strcmp(l->GetType(), "string") == 0) && (strcmp(r->GetType(), "string") == 0)) {
            loc = generator->GenBuiltInCall(StringEqual, l, r);
        }
        else {
            loc = generator->GenBinaryOp(op->tokenString, left->Eval(), right->Eval());
        }
    }
    loc->SetType("bool");
    return loc;
} 

Location * LogicalExpr::Eval() {
    Location * loc;
    if (strcmp(op->tokenString, "!") == 0) {
        Location * two = generator->GenLoadConstant(2);
        Location * one = generator->GenLoadConstant(1);
        loc = generator->GenBinaryOp("%", generator->GenBinaryOp("+", right->Eval(), one), two);
    }
    else {
        loc = generator->GenBinaryOp(op->tokenString, left->Eval(), right->Eval());
    }
    loc->SetType("bool");
    return loc;
} 

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

Location * ArrayAccess::Eval() {
    return this->Eval(false);
}

Location * ArrayAccess::Eval(bool returnAddr) {
    Location * index = subscript->Eval();
    Location * arr = base->Eval();
    Location * zero = generator->GenLoadConstant(0);
    Location * length = generator->GenLoad(arr);
    char * isInRange = generator->NewLabel();
    generator->GenIfZ(generator->GenBinaryOp("||", generator->GenBinaryOp("||", generator->GenBinaryOp("<", length, index), generator->GenBinaryOp("==", length, index)), generator->GenBinaryOp("<", index, zero)), isInRange);
    Location * error = generator->GenLoadConstant(err_arr_out_of_bounds);
    generator->GenBuiltInCall(PrintString, error);
    generator->GenBuiltInCall(Halt);
    generator->GenLabel(isInRange);
    Location * varSize = generator->GenLoadConstant(generator->VarSize);
    Location * offset = generator->GenBinaryOp("*", index, varSize);
    Location * loc = generator->GenBinaryOp("+", arr, offset);
    if (!returnAddr) {
        loc = generator->GenLoad(loc, 4);
        loc->SetType(arr->GetElemType());
    }
    return loc;
}
     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

Location * FieldAccess::Eval() {
    return this->Eval(false);
}

Location * FieldAccess::Eval(bool returnAddr) {
    ClassLookup * classInfo = NULL;
    Location * baseLoc = NULL;
    if (!base && !this->GetTmpLocation(this->field->name)) {
        baseLoc = generator->ThisPtr;
    }
    if (base || baseLoc) {
        if (!baseLoc) {
            baseLoc = base->Eval();
        }
        if (strcmp(baseLoc->GetName(), "this") == 0) {
            Node * node = this->parent;
            ClassDecl * classDecl;
            while (node) {
                if (classDecl = dynamic_cast<ClassDecl *>(node)) {
                    classInfo = classLookups->Lookup(classDecl->id->name);
                    break;
                }
                node = node->parent;
            }
        }
        else {
            classInfo = classLookups->Lookup(baseLoc->GetType());
        }
    
        Location * offset = generator->GenLoadConstant(*(classInfo->fields->Lookup(field->name)));
        Location * loc = generator->GenBinaryOp("+", baseLoc, offset);
        if (!returnAddr) {
            loc = generator->GenLoad(loc, 4);
            loc->SetType(classInfo->types->Lookup(field->name));
            char * temp = new char[100];
            sprintf(temp, "%s_elem", field->name);
            if (classInfo->types->Lookup(temp)) loc->SetElemType(classInfo->types->Lookup(temp));
        }
        return loc;
    }
    return this->GetTmpLocation(this->field->name);
}


Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

Location * Call::Eval() {
    Expr * arg;
    Location * loc;
    ClassLookup * classInfo = NULL;
    Location * baseLoc = NULL;

    This * it = dynamic_cast<This *>(base);

    if (!base || it) {
        Node * node = this->parent;
        ClassDecl * classDecl;
        while (node) {
            if (classDecl = dynamic_cast<ClassDecl *>(node)) {
                classInfo = classLookups->Lookup(classDecl->id->name);
                break;
            }
            node = node->parent;
        }

        if (classInfo) {
            if (classInfo->methods->Lookup(field->name)) {
                baseLoc = generator->ThisPtr;
            }
        }
    }


    if (base || baseLoc) {
        if (baseLoc == NULL) baseLoc = base->Eval();
        if (base && baseLoc->GetType() && strcmp(baseLoc->GetType(), "array") == 0) {
            // base is an array, not an object
            if (strcmp(field->name, "length") == 0) {
                loc = generator->GenLoad(baseLoc);
                loc->SetType(baseLoc->GetElemType());
                return loc;
            }
        }
        else {
            if (classInfo == NULL) {
                classInfo = classLookups->Lookup(baseLoc->GetType());
            }
            Location * firstFuncAddr = generator->GenLoad(baseLoc);
            int * num = classInfo->methods->Lookup(field->name);
            Location * offset = generator->GenLoadConstant(*num);
            Location * funcAddr = generator->GenBinaryOp("+", firstFuncAddr, offset);
            Location * func = generator->GenLoad(funcAddr);
            for (int i=actuals->NumElements() -1; i >= 0; i--) {
                arg = actuals->Nth(i);
                loc = arg->Eval();
                generator->GenPushParam(loc);
            }
            generator->GenPushParam(baseLoc);
            Location * loc = generator->GenACall(func, true);
            loc->SetType(classInfo->types->Lookup(field->name));
            char * temp = new char[100];
            sprintf(temp, "%s_elem", field->name);
            if (classInfo->types->Lookup(temp)) loc->SetElemType(classInfo->types->Lookup(temp));
            return loc;
        }
    }

    classInfo = classLookups->Lookup("&global");
    for (int i=actuals->NumElements() -1; i >= 0; i--) {
        arg = actuals->Nth(i);
        loc = arg->Eval();
        generator->GenPushParam(loc);
    }
    loc = generator->GenLCall(field->name, true);
    generator->GenPopParams(generator->VarSize * actuals->NumElements());
    char * funcType = classInfo->types->Lookup(field->name);
    char * temp = new char[100];
    sprintf(temp, "%s_elem", field->name);
    if (classInfo->types->Lookup(temp)) loc->SetElemType(classInfo->types->Lookup(temp));
    loc->SetType(funcType);
    return loc; 
}

Location * AssignExpr::Eval() {
    ArrayAccess * array;
    FieldAccess * field;
    if (field = dynamic_cast<FieldAccess *>(left)){
        if (!this->GetTmpLocation(field->field->name) || field->base) {
            Location * val = right->Eval();
            Location * memAddr = field->Eval(true);
            generator->GenStore(memAddr, val, 4);
            return val;
        }
    }
    if (array = dynamic_cast<ArrayAccess *>(left)){
        Location * val = right->Eval();
        Location * memAddr = array->Eval(true);
        generator->GenStore(memAddr, val, 4);
        return val;
    }
    Location * loc1 = left->Eval();
    Location * loc2 = right->Eval();
    if (loc2->GetType()) {
        loc1->SetType(loc2->GetType());
    }
    generator->GenAssign(loc1, loc2);
    return loc1;
}


NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

Location * NewExpr::Eval() {
    ClassLookup * classInfo = classLookups->Lookup(cType->id->name);

    Location * bytesToAlloc = generator->GenLoadConstant((4 * classInfo->fieldCount) + 4);
    Location * allocatedAddr = generator->GenBuiltInCall(Alloc, bytesToAlloc);
    Location * vtable = generator->GenLoadLabel(cType->id->name);
    generator->GenStore(allocatedAddr, vtable);
    allocatedAddr->SetType(cType->id->name);
    return allocatedAddr;
}

Location * This::Eval() {
    return generator->ThisPtr;
}

Location * NullConstant::Eval() {
    Location * word = generator->GenLoadConstant(4);
    Location * mem = generator->GenBuiltInCall(Alloc, word);
    return mem;
}


NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this);
    (elemType=et)->SetParent(this);
}

Location * NewArrayExpr::Eval() {
    Location * arrLength = size->Eval();
    char * okayLabel = generator->NewLabel();
    Location * zero = generator->GenLoadConstant(0);
    generator->GenIfZ(generator->GenBinaryOp("<", arrLength, zero), okayLabel);
    Location * error = generator->GenLoadConstant(err_arr_bad_size);
    generator->GenBuiltInCall(PrintString, error);
    generator->GenBuiltInCall(Halt);
    generator->GenLabel(okayLabel);
    Location * one = generator->GenLoadConstant(1);
    Location * length = generator->GenBinaryOp("+", one, arrLength);
    Location * varSize = generator->GenLoadConstant(generator->VarSize);
    Location * bytes = generator->GenBinaryOp("*", length, varSize);
    Location * allocatedAddr = generator->GenBuiltInCall(Alloc, bytes);
    generator->GenStore(allocatedAddr, arrLength);
    allocatedAddr->SetElemType(GetATypeChar(elemType));
    allocatedAddr->SetType("array");
    return allocatedAddr;
}

Location * ReadLineExpr::Eval() {
    Location * string = generator->GenBuiltInCall(ReadLine);
    string->SetType("string");
    return string;
}

Location * ReadIntegerExpr::Eval() {
    Location * integer = generator->GenBuiltInCall(ReadInteger);
    integer->SetType("int");
    return integer;
}

       
