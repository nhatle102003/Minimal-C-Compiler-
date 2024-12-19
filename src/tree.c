#include<tree.h>

/* string values for ast node types, makes tree output more readable */
enum nodeTypes {PROGRAM, DECLLIST, DECL, VARDECL, TYPESPEC, FUNDECL,
                FORMALDECLLIST, FORMALDECL, FUNBODY, LOCALDECLLIST,
                STATEMENTLIST, STATEMENT, COMPOUNDSTMT, ASSIGNSTMT,
                CONDSTMT, LOOPSTMT, RETURNSTMT, EXPRESSION, RELOP,
                ADDEXPR, ADDOP, TERM, MULOP, FACTOR, FUNCCALLEXPR,
                ARGLIST, INTEGER, IDENTIFIER, VAR, ARRAYDECL, CHAR,
                FUNCTYPENAME};

char *nodeNames[33] = {"program", "declList", "decl", "varDecl", "typeSpecifier",
                       "funDecl", "formalDeclList", "formalDecl", "funBody",
                       "localDeclList", "statementList", "statement", "compoundStmt",
                       "assignStmt", "condStmt", "loopStmt", "returnStmt","expression",
                       "relop", "addExpr", "addop", "term", "mulop", "factor",
                       "funcCallExpr", "argList", "integer", "identifier", "var",
                       "arrayDecl", "char", "funcTypeName"};

char *typeNames[3] = {"int", "char", "void"};
char *ops[10] = {"+", "-", "*", "/", "<", "<=", "==", ">=", ">", "!="};

tree *maketree(int kind) {
      tree *this = (tree *) malloc(sizeof(struct treenode));
      this->nodeKind = kind;
      this->numChildren = 0;
      return this;
}

tree *maketreeWithVal(int kind, int val) {
      tree *this = (tree *) malloc(sizeof(struct treenode));
      this->nodeKind = kind;
      this->numChildren = 0;
      this->val = val;
      return this;
}

tree *maketreeWithChar(int kind, char character) {
      tree *this = (tree *) malloc(sizeof(struct treenode));
      this->nodeKind = kind;
      this->numChildren = 0;
      this->character = character;
      return this;
}

tree *makeIdentifierTree(int kind, int val, table_node *local_scope){
      tree *this = (tree *) malloc(sizeof(struct treenode));
      this->nodeKind = kind;
      this->numChildren = 0;
      this->val = val;
      this->scope = local_scope;
      return this;
}
void addChild(tree *parent, tree *child) {
      if (parent->numChildren == MAXCHILDREN) {
          printf("Cannot add child to parent node\n");
          exit(1);
      }
      nextAvailChild(parent) = child;
      parent->numChildren++;
}

void printAst(tree *root, int nestLevel){
    // If the current root is not null, then proceed
    if(root){
        // Determine the kind of node, then print it accordingly
        if(root->nodeKind == INTEGER){
            
            printf("%s, %d\n", "integer", root->val);
        }
        else if(root->nodeKind == CHAR){
             printf("%s, %c\n", "character", root->character);
        }
        else if (root->nodeKind == TYPESPEC)
        {
            printf("%s, %s\n", nodeNames[root->nodeKind], typeNames[root->val]);
        }
        else if (root->nodeKind == IDENTIFIER)
        {
            if(root->val == -1)
                printf("%s,%s\n", nodeNames[root->nodeKind] ,"undeclared variable");
            else if(root->val == -2)
                printf("%s,%s\n", nodeNames[root->nodeKind] ,"redeclared variable");
            else{
                if(root->scope->strTable[root->val] == NULL){ 
                    printf("%s, %s\n", nodeNames[root->nodeKind], global_scope->strTable[root->val]->id);
                }
                else{
                    printf("%s, %s\n", nodeNames[root->nodeKind], root->scope->strTable[root->val]->id);
                }
                
            }
            
        }
        else if (root->nodeKind == ADDOP || root->nodeKind == MULOP || root->nodeKind == RELOP){
            printf("%s, %s\n", nodeNames[root->nodeKind], ops[root->val]);
        }
        else{
            printf("%s\n", nodeNames[root->nodeKind]);
        }
        
    }
    int i, j;
    // Iterate through each of the root's children
    for(i = 0; i < root->numChildren; i++){
        // If the child node is not null, then proceed
        if(getChild(root, i)){
            // Print out the proper number of spaces to visualize the node level of the child
            for(j = 0; j < nestLevel; j++){
                printf("  ");
            }
            // Recursively print out the child nodes
            printAst(getChild(root, i), nestLevel + 1);
        }
    } 
}

void flattenList(tree *list, tree *subList){
    for(int i=0; i < subList->numChildren; i++){
        addChild(list,getChild(subList,i));
    }
}

