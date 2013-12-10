/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}

Location * Node::GetTmpLocation(const char *key) {
	Node * node = this;
	while (node) {
		if (node->scope){
			Location * location = node->scope->symtab->Lookup(key);
			if (location)
				return location;
		} 
		node = node->parent;
	}
	return NULL;
}
	 
Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
} 

