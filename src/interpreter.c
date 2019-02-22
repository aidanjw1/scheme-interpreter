#include <stdio.h>
#include <string.h>
#include "value.h"
#include "interpreter.h"
#include "talloc.h"
#include "assert.h"
#include "linkedlist.h"
#include "parser.h"

Value *lookUpSymbol(Value *symbol, Frame *frame) {
    Value *current = frame->bindings;
    while (current->type != NULL_TYPE) {
        if (!strcmp(car(car(current))->s, symbol->s)) {
            return car(cdr(car(current)));
        }
        current = cdr(current);
    }
    if (frame->parent != NULL){
        return lookUpSymbol(symbol, frame->parent);
    }
    else {
        printf("%s: undefined; cannot reference an identifier before its definition", symbol->s);
    }
}

bool symbolDefined(Value *symbol, Value *bindings) {
    Value *current = bindings;
    while (current->type != NULL_TYPE) {
        if (!strcmp(car(car(current))->s, symbol->s)) {
            return true;
        }
        current = cdr(current);
    }
    return false;
}

Value *evalIf(Value *args, Frame *frame) {
    if (length(args) != 3) {
        printf("if: bad syntax in if");
        texit(1);
    }
    Value *condition = eval(car(args), frame);
    if(condition->type != BOOL_TYPE) {
        printf("Evaluation Error");
        texit(1);
    }
    Value *first = car(cdr(args));
    Value *second = car(cdr(cdr(args)));
    if (!strcmp(condition->s, "#t")) {
        return eval(first, frame);
    }
    else {
        return eval(second, frame);
    }
}

Value *evalLet(Value* args, Frame *frame) {
    Frame *newFrame = talloc(sizeof(Frame));
    newFrame->parent = frame;
    newFrame->bindings = makeNull();

    Value *letBindings = car(args);
    Value *body = car(cdr(args));

    Value *current = letBindings;
    while(current->type != NULL_TYPE) {
        Value *binding = car(current);
        if(length(binding) == 2 && car(binding)->type == SYMBOL_TYPE) {
            if (symbolDefined(car(binding), newFrame->bindings)) {
                printf("let: duplicate identifier in: %s", car(binding)->s);
                texit(1);
            }
            newFrame->bindings = cons(binding, newFrame->bindings);
            current = cdr(current);
        }
        else {
            printf("let: bad syntax (not an identifier)");
            texit(1);
        }
    }

    return eval(body, newFrame);
}
void interpret(Value *tree) {
    Frame *global = talloc(sizeof(Frame));
    global->bindings = makeNull();
    global->parent = NULL;
    Value *current = tree;
    while(current->type != NULL_TYPE) {
        Value *result = eval(car(current), global);
        printTree(result);
        printf("\n");
        current = cdr(current);
    }
}
Value *eval(Value *expr, Frame *frame) {
    Value *newTree = makeNull();
    switch(expr->type) {
        case INT_TYPE: {
            return expr;
        }
        case DOUBLE_TYPE: {
            return expr;
        }
        case BOOL_TYPE: {
            return expr;
        }
        case STR_TYPE: {
            return expr;
        }
        case SYMBOL_TYPE: {
            return lookUpSymbol(expr, frame);
            break;
        }
        case CONS_TYPE: {
            Value *first = car(expr);
            Value *args = cdr(expr);

            // Error checking
            Value *result = makeNull();
            if (first->type != SYMBOL_TYPE) {
                Value *current = expr;
                while(current->type != NULL_TYPE) {
                    result = cons(eval(car(current), frame), result);
                    current = cdr(current);
                }
                return result;
            }
            else if (!strcmp(first->s,"if")) {
                result = evalIf(args,frame);
                return result;
            }
            else if (!strcmp(first->s,"let")) {
                result = evalLet(args,frame);
                return result;
            }
            return newTree;
        }
        default:
            break;
    }
    return newTree;
}
