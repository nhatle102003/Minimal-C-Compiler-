#include "strtab.h"

// Stores symbols for determining the type of ID
// "" -> SCALAR, ID is a standard var
// "[]" -> ARRAY, ID is an array
// "()" -> FUNCTION, ID is a function
char *symTypeMod[] = {"", "[]", "()"};

// Stores the valid types of vars for the ID
char *types[] = {"int", "char", "void"};

param *working_list_head = NULL;
param *working_list_end = NULL;
table_node* current_scope;
table_node* global_scope;

/* Provided is a hash function that you may call to get an integer back. */
// djb2 hash function taken from http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


int ST_insert(char *id, char* scope, int data_type, int symbol_type){
    
    int index = hash(id) % MAXIDS;
 
    int start = index;
    
    /*if the entry of the symTable at the index is not empty, then we do linear probing here...*/
    
    while(current_scope->strTable[index] != NULL){
        if(current_scope->strTable[index] != NULL)
            if (strcmp(current_scope->strTable[index]->id, id) == 0 && strcmp(current_scope->strTable[index]->scope, scope) == 0){
                yyerror("Symbol declared multiple times.");
                return -2;
            }
        if(current_scope->strTable[index]->id != NULL && (strcmp(current_scope->strTable[index]->id, id) != 0 || strcmp(current_scope->strTable[index]->scope, scope) != 0)){
            // ...then continue searching for a valid placement in the table
            index = (index + 1) % MAXIDS;
            // If the index loops back to the starting index, then the table is full and return an error
            if(index == start){
                yyerror("Symbol table is full");
                return -1;
            }
        }
            
    }
    
    /*...else we only exit the loop upon reaching an empty, the expected beahvior of this function will always return an index with all the field initialzie correctly*/

    if(current_scope->strTable[index] == NULL){
        current_scope->strTable[index] = (symEntry*)malloc(sizeof(symEntry));
        current_scope->strTable[index]->id = id;
        current_scope->strTable[index]->scope = scope;
        current_scope->strTable[index]->data_type = data_type;
        current_scope->strTable[index]->symbol_type = symbol_type;
        current_scope->strTable[index]->symIndex = index;
    }
    
    //printf("Inserted index is %d at scope %s and the id is %s \n", index, scope, id);
    return index;
    
    
}

symEntry * ST_lookup(char *id, char *scope) {
    unsigned long index = hash(id) % MAXIDS;
    int start = index;
    //printf("The current lookup scope is %s \n", scope);
    /* First, we assume the scope is local... */
    if (current_scope->strTable[index] != NULL){
        if(!strcmp(current_scope->strTable[index]->id, id) && !strcmp(current_scope->strTable[index]->scope, scope)){
            //printf("Look up index is %d \n", index);
            return current_scope->strTable[index];
        } 
    }

    /* If not found at this position, do linear probing here... */
    index = (index + 1) % MAXIDS;
    while (index != start) {
        if (current_scope->strTable[index] != NULL){
            if(!strcmp(current_scope->strTable[index]->id, id) &&!strcmp(current_scope->strTable[index]->scope, scope)){
                //printf("Look up index is %d \n", index);
                return current_scope->strTable[index];
            }
        }
        index = (index + 1) % MAXIDS;
    }

    /* If still not found, check the "global" scope */
    if (global_scope->strTable[index] != NULL){
        if(!strcmp(global_scope->strTable[index]->id, id) && !strcmp(global_scope->strTable[index]->scope, "global")){
            //printf("Look up index is %d \n", index);
            return global_scope->strTable[index];
        }
    }

    /* Linear probing for "global" scope */
    index = (index + 1) % MAXIDS;
    while (index != start) {
        if (global_scope->strTable[index] != NULL){
            if(!strcmp(global_scope->strTable[index]->id, id) && !strcmp(global_scope->strTable[index]->scope, "global")){
                //printf("Look up index is %d \n", index);
                return global_scope->strTable[index];
            }
        }
        index = (index + 1) % MAXIDS;
    }

    /* If not found, return NULL */
    return NULL;
}


void new_scope(){
    /*Initialize the scope of the program, should be global at the beginning*/
    current_scope = malloc(sizeof(table_node));
    current_scope->super_scope = (char*) malloc(100);
    current_scope->paramCount = 0;
    current_scope->numChildren = 0;
    
}

void up_scope(){
    /*Simply move the current scope back to the parent scope*/
    current_scope = current_scope->parent;
    
}
 
/*Add paramters to fucntion scope */
void addParam(symEntry* function,  int data_type, int symbol_type){
    // If the param list has not been initialized, allocate memory for it and add the first parameter
    if (function->working_list_head == NULL){
        function->params = malloc(sizeof(param));
        function->params->data_type = data_type;
        function->params->symbol_type = symbol_type;
        function->params->next = NULL;
        function->working_list_head = function->params;
        function->working_list_end = function->params;
        function->working_list_pointer = function->working_list_head; //util pointer to help traverse the arg lists
    // Otherwise, allocate memory for the parameter and add it to the list
    }else{
        param* newParam = malloc(sizeof(param));
        newParam ->data_type = data_type;
        newParam ->symbol_type = symbol_type;
        newParam->next = NULL;
        function->params->next = newParam;
        function->params = function->params->next;
        function->working_list_end = function->params;
    }
    // Increment the size of the function
    function->size++;
}

bool paramTypeChecking(symEntry* function, int data_type){
    // Fetch the parameter list of the function
    working_list_head = function->working_list_pointer;
    // If the parameter list exists, continue
    if(working_list_head != NULL){
        /* If the current parameter data type does not match the expected, iterate the parameter
        and return true */
        if (working_list_head->data_type != data_type){
            function->working_list_pointer = function->working_list_head;
            return true;
        }
        // If the initial parameter list exists, continue
        if(function->working_list_pointer != NULL){
            // If there is at least one additional parameter in the list, iterate the parameter 
            if(function->working_list_pointer->next != NULL)
                working_list_head = function->working_list_pointer->next;
        }
    }
    if(function->working_list_pointer == NULL){ //reset this back to the head node
            function->working_list_pointer = function->working_list_head;
            working_list_head = NULL;
        }
    // If the data type is valid, return false
    return false;
}

/*Handling type and symbol checking, incrementally traverse the linked list and check the argument 
type of each argument position one by one*/
bool paramTypeAndSymbolChecking(symEntry* function, int data_type, int symbol_type){
    // If the parameter list of the function exists, continue
    if(function->working_list_pointer != NULL){
        /* If the iterated parameter is not the proper data or symbol type, iterate the parameter
        and return true */
        if (function->working_list_pointer->data_type != data_type || function->working_list_pointer->symbol_type != symbol_type){
            function->working_list_pointer = function->working_list_head;
            return true;
        }
        // Iterate the parameter
        function->working_list_pointer = function->working_list_pointer->next; 
    }
    if(function->working_list_pointer == NULL){ //reset this back to the head node
        function->working_list_pointer = function->working_list_head;
        }
    // If the data type and symbol is valid, return false
    return false;
}

/*check type compability between the assignee and the asggnor */
bool assgnTypeChecking(symEntry* variable, int data_type){
    // If the variable exists and its data type is not void, continue
    if(variable != NULL && variable->data_type != VOID_TYPE){ 
        // If the variable data type does not match the assignment type, return true
        if(variable->data_type != data_type){
            return true;
        }
    }
    // If the assignment data type is valid, return false
    return false;
}

/*Add a supscope to the current scope */
void addChildToScope(table_node* parent, table_node* child){
    // If the parent's number of children is at max, exit
    if (parent->numChildren == MAXCHILDRENS) {
          printf("Cannot add child to parent node\n");
          exit(1);
      }
      nextAvailChildScope(parent) = child;
      parent->numChildren++;
}

void print_sym_table_util(table_node* root, int nestLevel){
    for(int j = 0; j < nestLevel; j++){
                printf("   ");
    }
    printf("\n %s Symbol Table: \n \n", root->super_scope);

    for(int i = 0; i < MAXIDS; i++){
        if(root->strTable[i] != NULL){
            // printf("%d: %s", i, types[root->strTable[i]->data_type]);
            // printf("%s: %s %s\n", root->parent->super_scope, root->strTable[i]->id, symTypeMod[root->strTable[i]->symbol_type]);
            for(int j = 0; j < nestLevel; j++){
                printf("        ");
            }
            printf("%d: %s ", i, types[root->strTable[i]->data_type]);
            if(root->strTable[i]->symbol_type == FUNCTION){
                printf(" %s: %s ", root->strTable[i]->scope, root->strTable[i]->id);
                printf("( ");
                param *pRoot = root->strTable[i]->working_list_head;
                while(pRoot){
                    printf(" %s %s", types[pRoot->data_type], symTypeMod[pRoot->symbol_type]);
                    pRoot = pRoot->next;
                }
                printf(" )\n") ;
            }
            else{
                printf(" %s: %s %s\n", root->strTable[i]->scope, root->strTable[i]->id, symTypeMod[root->strTable[i]->symbol_type]);
            }
            
            }  
        }
   

    // Iterate through each of the root's children
    for(int k = 0; k < root->numChildren; k++){
        // If the child node is not null, then proceed
        if(getScopeChild(root, k)){
            // Print out the proper number of spaces to visualize the node level of the child
            // Recursively print out the child nodes
            print_sym_table_util(getScopeChild(root, k), nestLevel + 1);
        }
    }     
}

void print_sym_tab(){
    printf("\n\nSYMBOL TABLE:\n");
    print_sym_table_util(symTableRoot, 1);
}
