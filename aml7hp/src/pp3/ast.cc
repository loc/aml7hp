/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf
#include "errors.h"


//Hashtable<Node *> hash;


Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}
	 
Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
} 

void Identifier::Check(Scope * scope){
	scope->CheckIfVariableDecl(name);
}

Inherit::Inherit(Inherit * parent = NULL) {
	this->activeScopes = new std::stack<Scope *>;
	this->scopes = new List<Scope *>;
	this->inheritables = new List<Inheritable *>;
	this->parent = parent;
}

Scope::Scope() {
	this->symtab = new Hashtable<Decl *>;
	this->children = new List<Scope *>;
}


void Scope::CheckVariableAlreadyDecl(const char *name) {
	Iterator<Decl*> iter = this->symtab->GetIterator();
	int count = 0;
	Decl * decl;
	Decl * decls[2];
	while ((decl = iter.GetNextValue()) != NULL) {
		if (strcmp(decl->GetId()->name, name) == 0) {
			decls[count] = decl;
			count++;
			if(count>2) break;
		}
	}

	if (count > 1) {
		ReportError::DeclConflict(decls[0],decls[1]);
	}
}

void Scope::CheckClassAlreadyDecl(const char *name) {
	this->CheckVariableAlreadyDecl(name);
}

void Scope::CheckInterfaceAlreadyDecl(const char *name) {
	this->CheckVariableAlreadyDecl(name);
}

void Scope::CheckFunctionAlreadyDecl(const char *name) {
	this->CheckVariableAlreadyDecl(name);
}

void Scope::CheckIfVariableDecl(const char *name) {
	// Scope * s = scope;
	// while (s->parent) {
	// 	if (s->symtab->Lookup(name)) {
	// 		break;
	// 	}
	// }
}
