/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf
#include "errors.h"
#include "ast_expr.h"


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
	// scope->CheckIfVariableDecl(name);
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
	this->interfaces = new List<Scope *>;
	this->extends = NULL;
	this->parent = NULL;
}


void Scope::CheckVariableAlreadyDecl(Decl *decl) {
	Iterator<Decl*> iter = this->symtab->GetIterator();
	int count = 0;
	Decl * check;
	while ((check = iter.GetNextValue()) != NULL) {
		// if (strcmp(decl->GetId()->name, name) == 0) {
		// 	decls[count] = decl;
		// 	count++;
		// 	if(count>2) break;
		// }
		//printf("%s, %s \n", decl->GetId()->name, check->GetId()->name);
		//printf("%d, %d \n\n", check->GetLocation()->first_line, decl->GetLocation()->first_line);

		if (check != decl && strcmp(decl->GetId()->name, check->GetId()->name) == 0) {
			if ((check->GetLocation()->first_line < decl->GetLocation()->first_line) || 
				(check->GetLocation()->first_line == decl->GetLocation()->first_line && 
					check->GetLocation()->first_column < decl->GetLocation()->first_column)) {
				ReportError::DeclConflict(decl,check);
			}

		}
	}
}

void Scope::CheckIfTypeExists(NamedType *decl){
	Iterator<Decl*> iter = this->symtab->GetIterator();
	bool found = false;
	Scope * scope = this;
	Decl* check = NULL;
	Decl* temp = NULL;

	
	while (scope != NULL) {
		while ((temp = iter.GetNextValue()) != NULL) {
			if (strcmp(decl->GetId()->name, temp->GetId()->name) == 0) {
				found = true;
				dynamic_cast<ClassDecl*>(temp);
				if (!temp) {
					found = false;
					break;
				}
			}
		}
		if (found) {
			break;
		}

		scope = scope->parent;
		if (scope) {
			iter = scope->symtab->GetIterator();
		}
	}
	if (!found) {
		ReportError::IdentifierNotDeclared(decl->GetId(),LookingForClass);
	}

}

void Scope::ThisInClassScope(This *node) {
	Node * parent = node->GetParent();
	ClassDecl * decl;
	bool found = false;

	while (parent != NULL) {
		decl = dynamic_cast<ClassDecl*>(parent);
		if (decl) {
			found = true;
			break;
		}
		parent = parent->GetParent();
	}

	if (!found) {
		ReportError::ThisOutsideClassScope(node);
	}
}

void Scope::CheckClassAlreadyDecl(Decl *decl) {
	this->CheckVariableAlreadyDecl(decl);
}

void Scope::CheckInterfaceAlreadyDecl(Decl *decl) {
	this->CheckVariableAlreadyDecl(decl);
}

void Scope::CheckFunctionAlreadyDecl(Decl *decl) {
	this->CheckVariableAlreadyDecl(decl);
}

void Scope::CheckIfVariableDecl(Decl *decl) {
	// Scope * s = scope;
	// while (s->parent) {
	// 	if (s->symtab->Lookup(name)) {
	// 		break;
	// 	}
	// }
}

bool VariableTypesEqual(VarDecl * a, VarDecl * b) {
	if (a->type->IsEquivalentTo(b->type) == false) {
		return false;
	}
	return true;
}

bool FunctionTypesEqual(FnDecl * a, FnDecl * b) {
	if (a->returnType->IsEquivalentTo(b->returnType) == false || 
		a->formals->NumElements() != b->formals->NumElements()) {
		return false;
	}
	for (int i=0;i < a->formals->NumElements(); i++) {
		if (VariableTypesEqual(a->formals->Nth(i), b->formals->Nth(i)) == false) {
			return false;
		}
	}
	return true;
}

void Scope::CheckFunctionOverridesProperly(Decl *decl) {
	Scope * interfaceScope;
	Scope * extendsScope = NULL;
	Decl* temp;

	for (int i=0;i < this->interfaces->NumElements(); i++) {
        interfaceScope = this->interfaces->Nth(i);
        Iterator<Decl*> iter = interfaceScope->symtab->GetIterator();

        while ((temp = iter.GetNextValue()) != NULL) {
        	if (strcmp(decl->GetId()->name, temp->GetId()->name) == 0) {
				if (!FunctionTypesEqual(dynamic_cast<FnDecl *>(decl), dynamic_cast<FnDecl *>(temp))) {
					ReportError::OverrideMismatch(decl);
					return;
				}
			}
        }
    }

    if (this->extends) {
    	extendsScope = this->extends;
    	Iterator<Decl*> iter = extendsScope->symtab->GetIterator();

        while ((temp = iter.GetNextValue()) != NULL) {
        	if (strcmp(decl->GetId()->name, temp->GetId()->name) == 0) {
				if (!FunctionTypesEqual(dynamic_cast<FnDecl *>(decl), dynamic_cast<FnDecl *>(temp))) {
					ReportError::OverrideMismatch(decl);
					return;
				}
			}
        }
    }

    if (this->parent) {
    	this->parent->CheckFunctionOverridesProperly(decl);
    }
}

Scope * Scope::FindScopeFromNamedType(NamedType * decl) {
	Iterator<Decl*> iter = this->symtab->GetIterator();
	Scope * scope = this;
	Decl * temp = NULL;

	while ((temp = iter.GetNextValue()) != NULL) {
		printf("%s, %s\n", decl->GetId()->name, temp->GetId()->name);
		if (strcmp(decl->GetId()->name, temp->GetId()->name) == 0) {
			return temp->scope;
		}
	}
	return NULL;
}

ClassDecl * Scope::FindClassDeclFromNamedType(NamedType * decl) {
	Iterator<Decl*> iter = this->symtab->GetIterator();
	Scope * scope = this;
	ClassDecl* check = NULL;
	Decl* temp = NULL;



	while (scope != NULL) {
		while ((temp = iter.GetNextValue()) != NULL) {
			if (strcmp(decl->GetId()->name, temp->GetId()->name) == 0) {
				check = dynamic_cast<ClassDecl*>(temp);
				if (check) {
					return check;
				}
			}
		}

		scope = scope->parent;
		if (scope) {
			iter = scope->symtab->GetIterator();
		}
	}
	return NULL;
}

InterfaceDecl * Scope::FindInterfaceDeclFromNamedType(NamedType * decl) {
	Iterator<Decl*> iter = this->symtab->GetIterator();
	Scope * scope = this;
	InterfaceDecl* check = NULL;
	Decl* temp = NULL;



	while (scope != NULL) {
		while ((temp = iter.GetNextValue()) != NULL) {
			if (strcmp(decl->GetId()->name, temp->GetId()->name) == 0) {
				check = dynamic_cast<InterfaceDecl *>(temp);
				if (check) {
					return check;
				}
			}
		}

		scope = scope->parent;
		if (scope) {
			iter = scope->symtab->GetIterator();
		}
	}
	return NULL;
}

