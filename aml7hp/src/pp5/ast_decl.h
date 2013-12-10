/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp5: You will need to extend the Decl classes to implement 
 * code generation for declarations.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "list.h"

class Type;
class NamedType;
class Identifier;
class Stmt;

class Decl : public Node 
{
  
  public:
    Identifier *id;
    Decl(Identifier *name);
    friend std::ostream& operator<<(std::ostream& out, Decl *d) { return out << d->id; }
    virtual void Emit() {};
    virtual void EnterScope() {};
    virtual void BuildLookups() {};
};

class VarDecl : public Decl 
{
  public:
    Type *type;
    VarDecl(Identifier *name, Type *type);
    void Emit();
};

class ClassDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;

  public:
    ClassDecl(Identifier *name, NamedType *extends, 
              List<NamedType*> *implements, List<Decl*> *members);
    void Emit();
    void EnterScope();
    ClassLookup * CreateClassLookup();
    void BuildLookups();
};

class InterfaceDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    
  public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);
    void Emit() {};
};

class FnDecl : public Decl 
{
  protected:
    List<VarDecl*> *formals;
    Stmt *body;
    
  public:
    Type *returnType;
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);
    void Emit();
    void BuildLookups();
};

#endif
