#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "linkedlist.h"
#include "value.h"
#include "talloc.h"
#include "assert.h"
#include "parser.h"


// Add the next token in the sequence to the parse tree (stack), creates subTrees when a close
// bracket or parentheses is found.
Value *addToParseTree(Value *tree, int *depth, int *depthB, Value *token) {
    if (token->type == CLOSE_TYPE && *depth != 0) {
        *depth -= 1;
        Value *subTree = makeNull();
        while (car(tree)->type != OPEN_TYPE) {
            subTree = cons(car(tree), subTree);
            tree = cdr(tree);
        }
        tree = cdr(tree);
        tree = cons(subTree, tree);
        return tree;
    }
    else if (token->type == CLOSE_TYPE && *depth == 0){
        printf("Syntax Error: Too many close parentheses.");
        texit(1);
    }
    else if (token->type == CLOSE_BRACKET_TYPE && *depthB != 0) {
        *depthB -= 1;
        Value *subTree = makeNull();
        while (car(tree)->type != OPEN_BRACKET_TYPE) {
            subTree = cons(car(tree), subTree);
            tree = cdr(tree);
        }
        tree = cdr(tree);
        tree = cons(subTree, tree);
        return tree;
    }
    else if (token->type == CLOSE_BRACKET_TYPE && *depthB == 0){
        printf("Syntax Error: Too many close brackets.");
        texit(1);
    }
    else if (token->type == OPEN_TYPE) {
        *depth += 1;
        return cons(token, tree);
    }
    else if (token->type == OPEN_BRACKET_TYPE) {
        *depthB += 1;
        return cons(token, tree);
    }
    return cons(token, tree);
}
// Takes a list of tokens from a Racket program, and returns a pointer to a
// parse tree representing that program.
Value *parse(Value *tokens) {
    Value *tree = makeNull();
    int depth = 0;
    int depthB = 0;
    Value *current = tokens;
    assert(current != NULL && "Error (parse): null pointer");
    while (current->type != NULL_TYPE) {
        Value *token = car(current);
        tree = addToParseTree(tree, &depth, &depthB, token);
        current = cdr(current);
    }
    if (depth != 0) {
        printf("Syntax Error: Not enough close parentheses.");
        texit(1);
    }
    else if (depthB != 0) {
        printf("Syntax Error: Not enough close brackets.");
        texit(1);
    }
    tree = reverse(tree);
    return tree;
}

//// Prints the tree to the screen in a readable fashion. It should look just like
//// Racket code; use parentheses to indicate subtrees.
//void printTree(Value *tree) {
//    if(tree->type != CONS_TYPE) {
//        switch (tree->type) {
//            case INT_TYPE:
//                printf("%i", tree->i);
//                break;
//            case DOUBLE_TYPE:
//                printf("%f", tree->d);
//                break;
//            case STR_TYPE:
//                printf("%s", tree->s);
//                break;
//            case BOOL_TYPE:
//                printf("%s", tree->s);
//                break;
//            case SYMBOL_TYPE:
//                printf("%s", tree->s);
//                break;
//            case CLOSURE_TYPE:
//                printf("#<procedure>");
//                break;
//            default:
//                break;
//        }
//    }
//
//    Value *newTree = tree;
//    while (newTree->type == CONS_TYPE) {
//        printf("(");
//        Value *carTree = car(newTree);
//        switch (carTree->type) {
//            case CONS_TYPE:
//                printTree(carTree);
//                break;
//            case INT_TYPE:
//                printf("%i", carTree->i);
//                break;
//            case DOUBLE_TYPE:
//                printf("%f", carTree->d);
//                break;
//            case STR_TYPE:
//                printf("%s", carTree->s);
//                break;
//            case BOOL_TYPE:
//                printf("%s", carTree->s);
//                break;
//            case SYMBOL_TYPE:
//                printf("%s", carTree->s);
//                break;
//            case DOT_TYPE:
//                printf("%s", carTree->s);
//                break;
//            case SINGLE_QUOTE_TYPE:
//                printf("%s", carTree->s);
//            default:
//                break;
//        }
//        newTree = cdr(newTree);
//        if (newTree->type != NULL_TYPE && carTree->type != SINGLE_QUOTE_TYPE) {
//            printf(" ");
//        }
//    }
//    printf(")");
//}

// Print the Value of a single token
void printToken(Value *token) {
    valueType type = token->type;
    switch(type) {
        case INT_TYPE:
            printf("%i", token->i);
            break;
        case DOUBLE_TYPE:
            printf("%f", token->d);
            break;
        case SYMBOL_TYPE:
            printf("%s", token->s);
            break;
        case STR_TYPE:
            printf("%s", token->s);
            break;
        case BOOL_TYPE:
            printf("%s", token->s);
            break;
        case CLOSURE_TYPE:
            printf("#<procedure>");
            break;
        case NULL_TYPE:
            printf("()");
            break;
        default:
            printf("Another token type");
            break;
    }
}

// Prints the tree to the screen in a readable fashion. It should look just like
// Racket code; use parentheses to indicate subtrees.
void printTree(Value *tree) {
    //assert(tree->type == CONS_TYPE);
    if(tree->type != CONS_TYPE) {
        printToken(tree);
        return;
    }
    printf("(");
    Value *current = tree;
    while(current->type == CONS_TYPE) {
        valueType carType = car(current)->type;
        if (carType == CONS_TYPE) {
            printTree(car(current));
        }
        else {
            printToken(car(current));
        }
        current = cdr(current);
        if (current->type != NULL_TYPE && current->type != SINGLE_QUOTE_TYPE) {
            printf(" ");
        }
    }
    if (current->type != NULL_TYPE) {
        printf(". ");
        printToken(current);
    }
    printf(")");
}

