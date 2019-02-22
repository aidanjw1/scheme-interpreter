#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "linkedlist.h"
#include "value.h"
#include "assert.h"
#include "talloc.h"

// Create a new NULL_TYPE value node.
Value *makeNull() {
    Value *val = talloc(sizeof(Value));
    val->type = NULL_TYPE;
    //val->marked = false;
    return val;
}

// Create a new CONS_TYPE value node.
Value *cons(Value *newCar, Value *newCdr) {
    Value *node = talloc(sizeof(Value));
    node->type = CONS_TYPE;
    //node->marked = false;
    node->c.car = newCar;
    node->c.cdr = newCdr;
    return node;
}

// Display the contents of the linked list to the screen in some kind of
// readable format
void display(Value *list) {
    printf("(");
    Value *testList = list;
    while (testList->type == CONS_TYPE) {
        Value *listCar = testList->c.car;
        switch (testList->c.car->type) {
            case INT_TYPE:
                printf("%i ", listCar->i);
                break;
            case DOUBLE_TYPE:
                printf("%f ", listCar->d);
                break;
            case STR_TYPE:
                printf("%s ", listCar->s);
                break;
            case CONS_TYPE:
                display(listCar);
                break;
            case PTR_TYPE:
                printf("%p", listCar->p);
                break;
            case NULL_TYPE:
                printf("() ");
                break;
            default:
                break;
        }
        testList = testList->c.cdr;
        if (testList->type != NULL_TYPE) {
            switch (testList->type) {
                case INT_TYPE:
                    printf(". %i ", testList->i);
                    break;
                case DOUBLE_TYPE:
                    printf(". %f ", testList->d);
                    break;
                case STR_TYPE:
                    printf(". %s ", testList->s);
                    break;
                case NULL_TYPE:
                    printf(". () ");
                    break;
                default:
                    break;
            }
        }
    }


    printf(")\n");
}

// Return a new list that is the reverse of the one that is passed in. All
// content within the list should be duplicated; there should be no shared
// memory between the original list and the new one.
Value *reverse(Value *list) {
    Value *prev = makeNull();
    Value *current = list;
    Value *next = cdr(list);
    Value *new;
    while(current->type != NULL_TYPE) {
        new = cons(car(current), prev);
        prev = new;
        current = next;
        if (current->type == NULL_TYPE) {
            break;
        } else {
            next = cdr(next);
        }
    }
    return prev;
}

// Utility to make it less typing to get car value.
Value *car(Value *list) {
    assert(list != NULL);
    return list->c.car;
}

// Utility to make it less typing to get cdr value.
Value *cdr(Value *list) {
    assert(list != NULL);
    return list->c.cdr;
}

// Utility to check if pointing to a NULL_TYPE value.
bool isNull(Value *value) {
    assert(value != NULL);
    return value->type == NULL_TYPE;
}

// Measure length of list.
int length(Value *value) {
    assert(value != NULL);
    int length = 0;
    Value *nextNode = value;
    while (nextNode->type != NULL_TYPE)
    {
        if (nextNode->type == CONS_TYPE) {
            nextNode = cdr(nextNode);
            ++length;
        } else {
            return ++length;
        }
    }
    return length;
}