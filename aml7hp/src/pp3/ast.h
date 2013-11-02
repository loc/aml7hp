/* File: ast.h
 * ----------- 
 * This file defines the abstract base class Node and the concrete 
 * Identifier and Error node subclasses that are used through the tree as 
 * leaf nodes. A parse tree is a hierarchical collection of ast nodes (or, 
 * more correctly, of instances of concrete subclassses such as VarDecl,
 * ForStmt, and AssignExpr).
 * 
 * Location: Each node maintains its lexical location (line and columns in 
 * file), that location can be NULL for those nodes that don't care/use 
 * locations. The location is typcially set by the node constructor.  The 
 * location is used to provide the context when reporting semantic errors.
 *
 * Parent: Each node has a pointer to its parent. For a Program node, the 
 * parent is NULL, for all other nodes it is the pointer to the node one level
 * up in the parse tree.  The parent is not set in the constructor (during a 
 * bottom-up parse we don't know the parent at the time of construction) but 
 * instead we wait until assigning the children into the parent node and then 
 * set up links in both directions. The parent link is typically not used 
 * during parsing, but is more important in later phases.
 *
 * Semantic analysis: For pp3 you are adding "Check" behavior to the ast
 * node classes. Your semantic analyzer should do an inorder walk on the
 * parse tree, and when visiting each node, verify the particular
 * semantic rules that apply to that construct.

 */

#ifndef _H_ast
#define _H_ast

#include <stdlib.h>   // for NULL
#include "location.h"
#include <iostream>
#include <stack>
#include "list.h"
#include "hashtable.h"


class Decl;
class ClassDecl;

class Scope {
  public:
    Hashtable<Decl *> * symtab;
    Scope * parent;
    List<Scope *> * children;
    List<Scope *> * interfaces;
    Scope * extends;
    Scope();
    void CheckVariableAlreadyDecl(const char *name);
    void CheckClassAlreadyDecl(const char *name);
    void CheckInterfaceAlreadyDecl(const char *name);
    void CheckFunctionAlreadyDecl(const char *name);
    void CheckIfVariableDecl(const char *name);
};

class Node 
{
  protected:
    yyltype *location;
    Node *parent;

  public:
    Scope * scope;
    Node(yyltype loc);
    Node();
    
    yyltype *GetLocation()   { return location; }
    void SetParent(Node *p)  { parent = p; }
    Node *GetParent()        { return parent; }
};
   

class Identifier : public Node 
{
    
  public:
    char *name;
    Identifier(yyltype loc, const char *name);
    friend std::ostream& operator<<(std::ostream& out, Identifier *id) { return out << id->name; }
    void Check(Scope * scope);
};

struct Inheritable {
  Scope * scope;
  ClassDecl * classDecl; 
};

class Inherit {
  public:
    std::stack<Scope *> * activeScopes;
    List<Scope *> * scopes;
    List<Inheritable *> * inheritables;
    Inherit * parent;
    Inherit(Inherit * parent);
};



// This node class is designed to represent a portion of the tree that 
// encountered syntax errors during parsing. The partial completed tree
// is discarded along with the states being popped, and an instance of
// the Error class can stand in as the placeholder in the parse tree
// when your parser can continue after an error.
class Error : public Node
{
  public:
    Error() : Node() {}
};



#endif
