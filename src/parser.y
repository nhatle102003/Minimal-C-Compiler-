%{
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<../src/tree.h>
#define YYDEBUG 1
extern int yylineno;

/* nodeTypes refer to different types of internal and external nodes that can be part of the abstract syntax tree.*/
enum nodeTypes {PROGRAM, DECLLIST, DECL, VARDECL, TYPESPEC, FUNDECL,
                FORMALDECLLIST, FORMALDECL, FUNBODY, LOCALDECLLIST,
                STATEMENTLIST, STATEMENT, COMPOUNDSTMT, ASSIGNSTMT,
                CONDSTMT, LOOPSTMT, RETURNSTMT, EXPRESSION, RELOP,
                ADDEXPR, ADDOP, TERM, MULOP, FACTOR, FUNCCALLEXPR,
                ARGLIST, INTEGER, IDENTIFIER, VAR, ARRAYDECL, CHAR,
                FUNCTYPENAME};

enum opType {ADD, SUB, MUL, DIV, LT, LTE, EQ, GTE, GT, NEQ};

/* NOTE: mC has two kinds of scopes for variables : local and global. Variables declared outside any
function are considered globals, whereas variables (and parameters) declared inside a function foo are local to foo. You should update the scope variable whenever you are inside a production that matches function definition (funDecl production). The rationale is that you are entering that function, so all variables, arrays, and other functions should be within this scope. You should pass this variable whenever you are calling the ST_insert or ST_lookup functions. This variable should be updated to scope = "" to indicate global scope whenever funDecl finishes. Treat these hints as helpful directions only. You may implement all of the functions as you like and not adhere to my instructions. As long as the directory structure is correct and the file names are correct, we are okay with it. */

int symIndex;
int numParam = 0;
symEntry *functionEntry = NULL;
bool already_caugth = false;
bool assgn_checking = false;
bool indexing = false;
symEntry *varEntry = NULL;
tree *ast;
table_node* symTableRoot;

%}

%debug

/* the union describes the fields available in the yylval variable */
%union
{
    int value;
    struct treenode *node;
    char *strval;
}

/*Add token declarations below. The type <value> indicates that the associated token will be of a value type such as integer, float etc., and <strval> indicates that the associated token will be of string type.*/
%token <strval> ID
%token <value> INTCONST
/* TODO: Add the rest of the tokens below.*/

%token <strval> STRCONST
%token <strval> CHARCONST

/*Reserved Keywords*/
%token <value> KWD_IF
%token <value> KWD_ELSE
%token <value> KWD_WHILE
%token <value> KWD_RETURN 

/*Reserved TypeSpecifier*/
%token <value> KWD_CHAR   
%token <value> KWD_INT
%token <value> KWD_VOID

/* operators */
%token <value> OPER_ADD    
%token <value> OPER_SUB    
%token <value> OPER_MUL   
%token <value> OPER_DIV   
%token <value> OPER_LT  
%token <value> OPER_GT    
%token <value> OPER_GTE   
%token <value> OPER_LTE  
%token <value> OPER_EQ    
%token <value> OPER_NEQ   
%token <value> OPER_ASGN    

/* brackets & parens */
%token <value> LSQ_BRKT    
%token <value> RSQ_BRKT  
%token <value> LCRLY_BRKT  
%token <value> RCRLY_BRKT 
%token <value> LPAREN     
%token <value> RPAREN    

/* punctuation */
%token <value> COMMA     
%token <value> SEMICLN  

/* special operators*/
%token <value> OPER_AT    
%token <value> OPER_INC   
%token <value> OPER_DEC  
%token <value> OPER_AND   
%token <value> OPER_OR    
%token <value> OPER_NOT   
%token <value> OPER_MOD   

/* comment codes */
%token <value>CMMNT          
%token <value>MULTLN_CMMNT    

/*error code*/
%token <value> ILLEGAL_TOK 
%token <value> ERROR     


/* TODO: Declate non-terminal symbols as of type node. Provided below is one example. node is defined as 'struct treenode *node' in the above union data structure. This declaration indicates to parser that these non-terminal variables will be implemented using a 'treenode *' type data structure. Hence, the circles you draw when drawing a parse tree, the following lines are telling yacc that these will eventually become circles in an AST. This is one of the connections between the AST you draw by hand and how yacc implements code to concretize that. We provide with two examples: program and declList from the grammar. Make sure to add the rest.  */

%type <node> program declList decl varDecl typeSpecifier funDecl formalDeclList formalDecl funBody localDeclList statementList statement compoundStmt assignStmt condStmt loopStmt returnStmt var expression relop addExpr addop term mulop factor funcCallExpr argList funcTypeName arrayDecl funcCallHeader


%start program

%%
/* TODO: Your grammar and semantic actions go here. We provide with two example productions and their associated code for adding non-terminals to the AST.*/

program     : {new_scope(); strcpy(current_scope->super_scope, "global"); global_scope = current_scope; symTableRoot = current_scope; ST_insert("output", "global", VOID_TYPE, FUNCTION);} declList
                {
                    tree* progNode = maketree(PROGRAM);
                    addChild(progNode, $2);
                    ast = progNode;
                }
                ;

declList    : decl
            {
                tree* declListNode = maketree(DECLLIST);
                addChild(declListNode, $1);
                $$ = declListNode;
            }
            | declList decl
            {
                tree* declListNode = maketree(DECLLIST);
                addChild(declListNode, $1);
                addChild(declListNode, $2);
                $$ = declListNode;
            }
            ;

decl:  varDecl
      {
          tree* declNode = maketree(DECL);
          addChild(declNode, $1);
          $$ = declNode;
      }
      | funDecl
      { 
          tree* declNode = maketree(DECL);
          addChild(declNode, $1);
          $$ = declNode;
      }
      ;


varDecl         : typeSpecifier ID 
                {
                    // For any ID, call ST_insert to add it to the hash table
                  symIndex = ST_insert($2, current_scope->super_scope, $1->val, ARRAY);
                  
                }
                LSQ_BRKT arrayDecl RSQ_BRKT SEMICLN
                {
                  tree* varDeclNode = maketree(VARDECL);
                  addChild(varDeclNode, $1);
                  addChild(varDeclNode, makeIdentifierTree(IDENTIFIER, symIndex, current_scope));
                  addChild(varDeclNode, $5);
                  $$ = varDeclNode;
                }
                | typeSpecifier ID SEMICLN
                {
                  tree* varDeclNode = maketree(VARDECL);
                  addChild(varDeclNode, $1);
                  symIndex = ST_insert($2, current_scope->super_scope, $1->val, SCALAR);
                  addChild(varDeclNode, makeIdentifierTree(IDENTIFIER, symIndex, current_scope));
                  $$ = varDeclNode;
                } 
                ;


typeSpecifier   : KWD_INT
                {
                  $$ = maketreeWithVal(TYPESPEC, INT_TYPE);
                }
                | KWD_CHAR
                {
                  $$ = maketreeWithVal(TYPESPEC, CHAR_TYPE);
                }
                | KWD_VOID
                { 
                  $$ = maketreeWithVal(TYPESPEC, VOID_TYPE);
                }
                ;

funDecl         : funcTypeName LPAREN formalDeclList RPAREN funBody
                {
                  tree* funcDeclNode = maketree(FUNDECL);
                  addChild(funcDeclNode, $1);
                  addChild(funcDeclNode, $3);
                  addChild(funcDeclNode, $5);
                  $$ = funcDeclNode;
                }
                | funcTypeName LPAREN RPAREN funBody
                {
                  tree* funcDeclNode = maketree(FUNDECL);
                  addChild(funcDeclNode, $1);
                  addChild(funcDeclNode, $4);
                  $$ = funcDeclNode;
                }
                ;

funcTypeName    : typeSpecifier ID
                {
                  tree* funcTypeNameNode = maketree(FUNCTYPENAME);
                  addChild(funcTypeNameNode, $1);
                  symIndex = ST_insert($2, current_scope->super_scope, $1->val, FUNCTION);
                  addChild(funcTypeNameNode, makeIdentifierTree(IDENTIFIER, symIndex, current_scope));
                  table_node *current_super_scope = current_scope; 
                  new_scope(); strcpy(current_scope->super_scope, $2); current_scope->parent = current_super_scope;
                  addChildToScope(current_scope->parent, current_scope);
                  $$ = funcTypeNameNode;
                }
                ;

formalDeclList  : formalDecl 
                {
                  tree* formalDeclListNode = maketree(FORMALDECLLIST);
                  addChild(formalDeclListNode, $1);
                  $$ = formalDeclListNode;
                }
                | formalDecl COMMA formalDeclList 
                {
                  tree* formalDeclListNode = maketree(FORMALDECLLIST);
                  addChild(formalDeclListNode, $1);
                  addChild(formalDeclListNode, $3);
                  $$ = formalDeclListNode;
                }
                ;

formalDecl      : typeSpecifier ID 
                {
                  tree* formalDeclNode = maketree(FORMALDECL);
                  addChild(formalDeclNode, $1);
                  symIndex = ST_insert($2, current_scope->super_scope, $1->val, SCALAR);
                  symEntry* funcEntry = ST_lookup(current_scope->super_scope, current_scope->parent->super_scope);
                  // If the function entry returns null, it has not been declared
                  if(funcEntry == NULL){
                      yyerror("Undeclared function");
                  }else{
                    addParam(funcEntry, $1->val, SCALAR);
                  }
                  
                  addChild(formalDeclNode, makeIdentifierTree(IDENTIFIER, symIndex, current_scope));
                  $$ = formalDeclNode;
                }
                | typeSpecifier ID 
                {
                   symIndex = ST_insert($2, current_scope->super_scope, $1->val, ARRAY);
                   symEntry* funcEntry = ST_lookup(current_scope->super_scope, current_scope->parent->super_scope);
                  // If the function entry returns null, it has not been declared
                  if(funcEntry == NULL){
                      yyerror("Undeclared function");
                  }else{
                    addParam(funcEntry, $1->val, ARRAY);
                  }
                }
                LSQ_BRKT arrayDecl RSQ_BRKT 
                {
                    tree* formalDeclNode = maketree(FORMALDECL);
                    symEntry* arrayEntry = ST_lookup($2, current_scope->super_scope);
                    if(arrayEntry){
                        for(int i = 0; i < $5->numChildren; i++){
                            tree *childNode = $5->children[i];
                            if(childNode->nodeKind == INTEGER){
                                arrayEntry->size = childNode->val;
                            }
                        }
                    }
                    addChild(formalDeclNode, $1);
                    addChild(formalDeclNode, makeIdentifierTree(IDENTIFIER, symIndex, current_scope));
                    addChild(formalDeclNode, $5);
                    $$ = formalDeclNode;
                }
                ;

arrayDecl       :
                 {
                  $$ = maketree(ARRAYDECL);
                 }
                 | INTCONST
                 {
                    // Arrays cannot be declared with a size of zero
                    if($1 == 0 && !indexing){
                      yyerror("Array variable declared with size of zero.");
                    }
                    else if(varEntry != NULL){
                        // Arrays cannot be declared with an out of bounds index
                        if($1 >= varEntry->size){
                            yyerror("Array index out-of-bounds");
                        }
                    }
                      tree*  arrayDeclNode = maketree(ARRAYDECL);
                      addChild(arrayDeclNode, maketreeWithVal(INTEGER, $1));
                      $$ = arrayDeclNode;
                 }
                 |CHARCONST
                 {
                    // Arrays cannot be declared with a char constant
                    yyerror("Array indexed using non-integer expression");
                    tree*  arrayDeclNode = maketree(ARRAYDECL);
                    addChild(arrayDeclNode, maketreeWithChar(CHAR, yylval.value));
                    $$ = arrayDeclNode;
                 }
                 |addExpr
                 {
                    tree*  arrayDeclNode = maketree(ARRAYDECL);
                    addChild(arrayDeclNode, $1);
                    $$ = arrayDeclNode;
                 }
                 |ID
                 {
                  symEntry *returnID = ST_lookup($1, current_scope->super_scope);
                  // If the ID is undeclared, throw an error
                  if(!returnID){
                    tree*  arrayDeclNode = maketree(ARRAYDECL);
                    addChild(arrayDeclNode , makeIdentifierTree(IDENTIFIER, -1, current_scope));
                    $$ = arrayDeclNode;
                    yyerror("Array indexed with undeclared variable.");
                  }
                  // If the ID is not an int, throw an error
                  else if(returnID->data_type != INT_TYPE){
                    tree*  arrayDeclNode = maketree(ARRAYDECL);
                    addChild(arrayDeclNode , makeIdentifierTree(IDENTIFIER, returnID->symIndex, current_scope));
                    $$ = arrayDeclNode;
                    yyerror("Array indexed using non-integer expression");
                  }
                  else{
                    tree*  arrayDeclNode = maketree(ARRAYDECL);
                    addChild(arrayDeclNode, makeIdentifierTree(IDENTIFIER, returnID->symIndex, current_scope));
                    $$ = arrayDeclNode;
                  }
                 }
                 ;

funBody         : LCRLY_BRKT localDeclList statementList RCRLY_BRKT
                {
                  tree* funBodyNode = maketree(FUNBODY);
                  addChild(funBodyNode, $2);
                  addChild(funBodyNode, $3);
                  $$ = funBodyNode;
                  up_scope();
                }
                ;

localDeclList   : 
                  {
               
                    $$ = NULL;
                  }
                | varDecl localDeclList
                {
                  tree* localDeclListNode = maketree(LOCALDECLLIST);
                  addChild(localDeclListNode, $1);
                  addChild(localDeclListNode, $2);
                  $$ = localDeclListNode;
                }
                | funDecl localDeclList
                {
                  tree* localDeclListNode = maketree(LOCALDECLLIST);
                  addChild(localDeclListNode, $1);
                  addChild(localDeclListNode, $2);
                  $$ = localDeclListNode;
                }
                ;
            
statementList   : 
                {
                  $$ = NULL;
                }
                | statement statementList
                {
                  tree* statementListNode = maketree(STATEMENTLIST);
                  addChild(statementListNode, $1);
                  addChild(statementListNode, $2);
                  $$ = statementListNode;
                }
                ;
                
statement       : compoundStmt
                {
                    tree* statementNode = maketree(STATEMENT);
                    addChild(statementNode, $1);
                    $$ = statementNode;
                }
                | assignStmt
                {
                  tree* statementNode = maketree(STATEMENT);
                  addChild(statementNode, $1);
                  $$ = statementNode;
                }
                | condStmt
                {
                  tree* statementNode = maketree(STATEMENT);
                  addChild(statementNode, $1);
                  $$ = statementNode;
                }
                | loopStmt
                {
                  tree* statementNode = maketree(STATEMENT);
                  addChild(statementNode, $1);
                  $$ = statementNode;
                }
                | returnStmt
                {
                  tree* statementNode = maketree(STATEMENT);
                  addChild(statementNode, $1);
                  $$ = statementNode;
                }
                ;

compoundStmt : LCRLY_BRKT statementList RCRLY_BRKT
                {
                   tree* compoundStmtNode = maketree(COMPOUNDSTMT);
                   addChild(compoundStmtNode, $2);
                   $$ = compoundStmtNode;
                }
                ;

assignStmt   : var OPER_ASGN {assgn_checking = true;} expression SEMICLN
            {
              tree* assignStmtNode = maketree(ASSIGNSTMT);
              addChild(assignStmtNode, $1);
              addChild(assignStmtNode, $4);
              varEntry = NULL;
              already_caugth = false;
              assgn_checking = false;
              $$ = assignStmtNode;
            }
            | expression SEMICLN
            {
              tree* assignStmtNode = maketree(ASSIGNSTMT);
              addChild(assignStmtNode, $1);
              varEntry = NULL;
              already_caugth = false;
              $$ = assignStmtNode;
            }
            ;

condStmt    : KWD_IF LPAREN expression RPAREN statement
            {
              tree* condStmtNode = maketree(CONDSTMT);
              addChild(condStmtNode, $3);
              addChild(condStmtNode, $5);
              $$ = condStmtNode;
            }
            | KWD_IF LPAREN expression RPAREN statement KWD_ELSE statement
            {
              tree* condStmtNode = maketree(CONDSTMT);
              addChild(condStmtNode, $3);
              addChild(condStmtNode, $5);
              addChild(condStmtNode, $7);
              $$ = condStmtNode;
            }
            ;

loopStmt    : KWD_WHILE LPAREN expression RPAREN statement
            {
              tree* loopStmtNode = maketree(LOOPSTMT);
              addChild(loopStmtNode, $3);
              addChild(loopStmtNode, $5);
              $$ = loopStmtNode;
            }
            ;

returnStmt  : KWD_RETURN SEMICLN
            {
              tree* returnStmtNode = maketree(RETURNSTMT);
              $$ = returnStmtNode;
            }
            | KWD_RETURN expression SEMICLN
            {
              tree* returnStmtNode = maketree(RETURNSTMT);
              addChild(returnStmtNode, $2);
              $$ = returnStmtNode;
            }
            ;

var        : ID
            {
              tree* varNode = maketree(VAR);
              symEntry* returnID = ST_lookup($1, current_scope->super_scope);
              // Check if the ID matches the required data type
              if(varEntry != NULL && returnID != NULL && assgn_checking){
                //an integer varible can be assigned the type char (treat them as integer)
                if(varEntry->data_type == INT_TYPE && returnID->data_type == CHAR_TYPE){ 
                    goto idBody;
                }
                if(assgnTypeChecking(varEntry, returnID->data_type) && !already_caugth){
                    yyerror("Type mismatch in assignment.");
                    already_caugth = true;
                }
                
              }
              idBody:
                varEntry = returnID;
                // Check if the ID matches the required parameter data type
                if(functionEntry != NULL && returnID != NULL){
                  if(paramTypeAndSymbolChecking(functionEntry, returnID->data_type, returnID->symbol_type) && !already_caugth){
                      yyerror("Argument type mismatch in function call.");
                      functionEntry->working_list_pointer = functionEntry->working_list_head;
                      already_caugth = true;
                  }
                }
                // If the ID returns null, it is undeclared
                if(returnID == NULL){
                  addChild(varNode, makeIdentifierTree(IDENTIFIER, -1, current_scope));
                  $$ = varNode;
                  yyerror("Undeclared variable");
                  yyerror("Type mismatch in assignment.");
                }
                else{
                  addChild(varNode, makeIdentifierTree(IDENTIFIER, returnID->symIndex, current_scope));
                  $$ = varNode;
                }
              
            }
            |ID LSQ_BRKT {indexing = true;} arrayDecl RSQ_BRKT
            {
              symEntry* returnID = ST_lookup($1, current_scope->super_scope);
              // Check if the ID matches the required data type
              if(varEntry != NULL && returnID != NULL){
                if(varEntry->data_type == INT_TYPE && returnID->data_type == CHAR_TYPE){ 
                    goto idArrBody;
                }
                if(assgnTypeChecking(varEntry, returnID->data_type) && !already_caugth){
                    yyerror("Type mismatch in assignment.");
                    already_caugth = true;
                } 
              }
              idArrBody:
                varEntry = returnID;
                tree* varNode = maketree(VAR);
                // Func for detecting tree children
                for(int i = 0; i < $4->numChildren; i++){
                  // Set the node to test with the current child node at index i
                  tree * childNode = $4->children[i];
                  // If the child is an add expression, continue
                  if(childNode->nodeKind == ADDEXPR){
                    // Check if the add expression value is out of bounds of the array declaration
                    for(int j = 0; j < childNode->numChildren; j++){
                      // If the iterated child node is an integer, continue
                      if(childNode->children[j]->nodeKind == INTEGER){
                        // If the value stored is not equal to the size stored in returnID, throw an error
                        if(childNode->children[j]->val != returnID->size){
                          yyerror("Statically sized array indexed with constant, out-of-bounds expression.");
                          already_caugth = true;
                        }
                      }
                    }
                  }
                }
                // Check if the ID matches the required parameter data type
                if(functionEntry != NULL && returnID != NULL){
                  if(paramTypeAndSymbolChecking(functionEntry, returnID->data_type, returnID->symbol_type) && !already_caugth){
                      yyerror("Argument type mismatch in function call.");
                      functionEntry->working_list_pointer = functionEntry->working_list_head;
                      already_caugth = true;
                  }
                }
                // If the ID returns null, it is undeclared
                else if(returnID == NULL){
                    addChild(varNode, makeIdentifierTree(IDENTIFIER, -1, current_scope)); goto var_arr_end;
                    yyerror("Undeclared variable");
                }
                else if(returnID->symbol_type != ARRAY){
                    yyerror("Non-array identifier used as an array.");
                }
                
                addChild(varNode, makeIdentifierTree(IDENTIFIER, returnID->symIndex, current_scope));
                var_arr_end:
                        addChild(varNode, $4);
                        $$ = varNode;
                        indexing = false;
            }
            ;

expression  : addExpr
            {
              tree* exprNode = maketree(EXPRESSION);
              addChild(exprNode, $1);
              $$ = exprNode;
            }
            | expression relop addExpr
            {
              tree* exprNode = maketree(EXPRESSION);
              addChild(exprNode, $1);
              addChild(exprNode, $2);
              addChild(exprNode, $3);
              $$ = exprNode;
            }
            ;

relop       : OPER_LTE
            {
              $$ = maketreeWithVal(RELOP, LTE);
            }
            | OPER_LT
            {
              $$ = maketreeWithVal(RELOP, LT);
            }
            | OPER_GT
            {
              $$ = maketreeWithVal(RELOP, GT);
            }
            | OPER_GTE
            { 
              $$ = maketreeWithVal(RELOP, GTE);
            }
            | OPER_EQ
            {
              $$ = maketreeWithVal(RELOP, EQ);
            }
            | OPER_NEQ
            {
              $$ = maketreeWithVal(RELOP, NEQ);
            }
            ;

addExpr     : term
            {   
              tree* addExprNode = maketree(ADDEXPR);
              addChild(addExprNode, $1);
              $$ = addExprNode;
            }
            | addExpr addop term
            {
              tree* addExprNode = maketree(ADDEXPR);
              // Properly divy the addExpr between the nodes and calculate the result for add/sub ops
              int *LHS = 0; int *RHS = 0; int res;
              // While the addExpr has child values, continue and iterate through them
              for(int i = 0; i < $1->numChildren; i++){
                  // Set current child node at index i
                  tree *childNode = $1->children[i];
                  // If the child node is a term, continue
                  if(childNode->nodeKind == TERM){
                    // Iterate through the current child's own children
                    for(int j = 0; j < childNode->numChildren; j++){
                        // Set current term child node at index j
                        tree* termChildren = childNode->children[j];
                        // If the term child is an integer, set the LHS to its value
                        if(termChildren->nodeKind == INTEGER){
                          LHS = &(termChildren->val);
                        }
                        // Otherwise, if the term child is a factor, iterate through the term's children
                        else if(termChildren->nodeKind == FACTOR){
                          for(int k = 0; k < termChildren->numChildren; k++){
                              // Set current factor child node at index k
                              tree* factorChildren = termChildren->children[k];
                              // If the factor child is an integer, set the LHS to its value
                              if(factorChildren->nodeKind == INTEGER){
                                LHS = &(factorChildren->val);
                              }
                          }
                          
                        }
                    }
                  }
              }
              // While the term has child values, continue and iterate through them
              for(int i = 0; i < $3->numChildren; ++i){
                  // Set current child node at index i
                  tree *childNode = $3->children[i];
                  // If the child is a factor itself, iterate through its own children
                  if(childNode->nodeKind == FACTOR){
                    for(int j = 0; j < childNode->numChildren; ++j){
                        // If one of the child nodes is an integer, set the RHS to its value
                        if(childNode->children[j]->nodeKind == INTEGER){
                          RHS = &(childNode->children[j]->val);
                        }
                    }
                  }
              }
              // If both LHS and RHS are non-zero, continue
              if(RHS != 0 && LHS != 0){
                  // If it's an add op, add LHS and RHS
                  if($2->val == ADD){
                    res = *LHS + *RHS ;
                  }
                  // If it's a sub op, subtract LHS from RHS
                  else if ($2->val == SUB){
                    res =  *LHS - *RHS;
                  }
                  addChild(addExprNode, maketreeWithVal(INTEGER, res));
              }
              else{
                addChild(addExprNode, $1);
                addChild(addExprNode, $2);
                addChild(addExprNode, $3);
              }
              
              $$ = addExprNode;
            }
            ;

addop       : OPER_ADD
            {
              $$ = maketreeWithVal(ADDOP, ADD);
            }
            | OPER_SUB
            {
              $$ = maketreeWithVal(ADDOP, SUB);
            }
            ;

term        : factor
            {
              tree* termNode = maketree(TERM);
              addChild(termNode, $1);
              $$ = termNode;
            }

            | term mulop factor
            {
              tree* termNode = maketree(TERM);
              // Properly divy the term between the nodes and calculate the result for mul/div ops
              int *LHS = 0;
              int *RHS = 0;
              int result;
              // While the term has child values, continue and iterate through them
              for(int i = 0; i < $1->numChildren; i++){
                // Set current child node at index i
                tree *childNode = $1->children[i];
                // If the child node is a factor, iterate through the child node's own children
                if(childNode->nodeKind == FACTOR){
                    for(int j = 0; j < childNode->numChildren; j++){
                        // If the child node at index j exists, continue
                        if(childNode->children[j]){
                            // If the node at index j is an integer, set LHS to its value
                            if(childNode->children[j]->nodeKind == INTEGER){
                              LHS = &(childNode->children[j]->val);
                            }
                        }  
                        
                    }
                }
              }
              // While the factor has child values, continue and iterate through them
              for(int i = 0; i < $3->numChildren; i++){
                // If the child node at index i is an integer, set RHS to its value
                if($3->children[i]->nodeKind == INTEGER){
                    RHS = &($3->children[i]->val);
                }
              }
              // If both LHS and RHS are non-zero, continue
              if(LHS != 0  && RHS != 0){
                // If it's a mul op, multiply the 2 values
                if($2->val == MUL){
                  result = (*LHS) * (*RHS);
                }
                else{
                // IF it's a div op, divide LHS by RHS
                  result = (*LHS) / (*RHS);
                }
                addChild(termNode, maketreeWithVal(INTEGER, result));
              }
              else{
                addChild(termNode, $1);
                addChild(termNode, $2);
                addChild(termNode, $3);

              }
              $$ = termNode;
            }
            ;

mulop       : OPER_MUL
            {
              $$ = maketreeWithVal(MULOP, MUL);
            }
            | OPER_DIV
            {
              $$ = maketreeWithVal(MULOP, DIV);
            }
            ;

factor      : LPAREN expression RPAREN
            {
              tree *factorNode = maketree(FACTOR);
              addChild(factorNode, $2);
              $$ = factorNode;
            }
            | var
            {
              tree *factorNode = maketree(FACTOR);
              addChild(factorNode, $1);
              $$ = factorNode;
            }
            | funcCallExpr
            {
              tree *factorNode = maketree(FACTOR);
              addChild(factorNode, $1);
              $$ = factorNode;
            }
            | INTCONST
            {
              tree* factorNode = maketree(FACTOR);
              addChild(factorNode, maketreeWithVal(INTEGER, $1));
              // Check if the int is being passed properly as an int parameter
              if(functionEntry != NULL){
                if(paramTypeChecking(functionEntry, INT_TYPE) && !already_caugth){
                      yyerror("Argument type mismatch in function call.");
                      functionEntry->working_list_pointer = functionEntry->working_list_head;
                      already_caugth = true;
                }
              }
              // Check if the int is being assigned to an int var
              if(varEntry != NULL){
                if(assgnTypeChecking(varEntry, INT_TYPE) && !already_caugth){
                  yyerror("Type mismatch in assignment.");
                  already_caugth = true;
                }
              }
              $$ = factorNode;
            }
            | CHARCONST
            {
              tree* factorNode = maketree(FACTOR);
              // Check if the char is being passed properly as a char parameter
              if(functionEntry != NULL){
                if(paramTypeChecking(functionEntry, CHAR_TYPE) && !already_caugth){
                      yyerror("Argument type mismatch in function call.");
                      functionEntry->working_list_pointer = functionEntry->working_list_head;
                      already_caugth = true;
                }
              }
              // Check if the char is being assigned to a char var
              if(varEntry != NULL){
                if(varEntry->data_type != INT_TYPE){
                  if(assgnTypeChecking(varEntry, CHAR_TYPE) && !already_caugth){
                      yyerror("Type mismatch in assignment.");
                      already_caugth = true;
                  }
                    
                }
              }
              addChild(factorNode, maketreeWithChar(CHAR, yylval.value));
              $$ = factorNode;
            }
            ;

funcCallExpr : funcCallHeader LPAREN argList RPAREN
              {
                    tree *funcCallExprNode = maketree(FUNCCALLEXPR);
                    // Check if the function contains too many or too few parameters
                    if(functionEntry != NULL){
                        if( functionEntry->size > numParam && !already_caugth){
                          yyerror("Too few arguments provided in function call.");
                          already_caugth = true;
                        }
                        else if( functionEntry->size < numParam && !already_caugth){
                            yyerror("Too many arguments provided in function call.");
                            already_caugth = true;
                        }
                    }
                    
                    addChild(funcCallExprNode, $1);
                    addChild(funcCallExprNode, $3); 
                    $$ = funcCallExprNode;
                    numParam = 0;
                    already_caugth = false;
                    functionEntry = NULL;
                    varEntry = NULL;
            }
            | funcCallHeader LPAREN RPAREN
                {
                  tree *funcCallExprNode = maketree(FUNCCALLEXPR);
                  // If the function has a parameter, throw an error
                  if(functionEntry != NULL){
                      if(numParam == 0 && functionEntry->size > 0){
                          yyerror("Too few arguments provided in function call.");
                          already_caugth = true;
                      }
                  }
                  addChild(funcCallExprNode, $1);
                  $$ = funcCallExprNode;
                  already_caugth = false;
                  functionEntry = NULL;
                  varEntry = NULL;
                }
                ;

funcCallHeader : ID
                {
                  //This is very specific to mC, we only care for the 
                  //output function
                  if(!strcmp($1, "output")){
                    $$ = makeIdentifierTree(IDENTIFIER, ST_lookup($1, "global")->symIndex, global_scope);
                  }
                  else{
                    functionEntry = ST_lookup($1, current_scope->super_scope);
                    // If the function does not exist, throw an error
                    if(!functionEntry){
                        $$ = makeIdentifierTree(IDENTIFIER, -1, current_scope);
                        yyerror("Undefined function");
                    }
                    else{
                      $$ = makeIdentifierTree(IDENTIFIER, functionEntry->symIndex, current_scope);
                    }
                  }
                  
                }

argList       : expression
                {
                  tree* argListNode = maketree(ARGLIST);
                  addChild(argListNode, $1);
                  numParam++;
                  $$ = argListNode;
                }
              | argList COMMA expression
                {
                  tree* argListNode = maketree(ARGLIST);
                  addChild(argListNode, $1);
                  addChild(argListNode, $3);
                  numParam++;
                  $$ = argListNode;
                }
                ;
%%

int yywarning(char * msg){
    printf("warning: line %d: %s\n", yylineno, msg);
    return 0;
}

int yyerror(char * msg){
  printf("error: line %d: %s\n", yylineno, msg);
  errorOccured();
  return 0;
}
