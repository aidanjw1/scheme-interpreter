#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "linkedlist.h"
#include "value.h"
#include "talloc.h"
#include "assert.h"

Value *activeList;

// Create a new NULL_TYPE value node.
Value *makeNullm() {
    Value *val = malloc(sizeof(Value));
    val->type = NULL_TYPE;
    //val->marked = false;
    return val;
}

// Create a new CONS_TYPE value node.
Value *consm(Value *newCar, Value *newCdr) {
    Value *node = malloc(sizeof(Value));
    node->type = CONS_TYPE;
    //node->marked = false;
    node->c.car = newCar;
    node->c.cdr = newCdr;
    return node;
}

// Replacement for malloc that stores the pointers allocated. It should store
// the pointers in some kind of list; a linked list would do fine, but insert
// here whatever code you'll need to do so; don't call functions in the
// pre-existing linkedlist.h. Otherwise you'll end up with circular
// dependencies, since you're going to modify the linked list to use talloc.
void *talloc(size_t size) {
    if (activeList == NULL) {
        activeList = makeNullm();
    }
    void *new = malloc(size);
    //new->marked = false;
    Value *p = malloc(sizeof(Value));
    p->type = PTR_TYPE;
    p->p = new;
    //p->marked = false;
    activeList = consm(p, activeList);
    return new;
}

// Free all pointers allocated by talloc, as well as whatever memory you
// allocated in lists to hold those pointers.
void tfree() {
    if (activeList == NULL) return;
    Value *new = NULL;
    Value *current = activeList;
    Value *next;
    while(current->type != NULL_TYPE) {
        next = cdr(current);
        Value* val = ((Value*)(car(current)->p));
        free(car(current)->p);
        free(car(current));
        free(current);
//        if (!val->marked){
//            free(car(current)->p);
//            free(car(current));
//            free(current);
//        }
//        else {
//            ((Value*)(car(current)->p))->marked = false;
//            if (new == NULL) new = makeNullm();
//            new = consm(car(current), new);
//            free(current);
//        }
        current = next;
    }
    free(current);
//    if (!current->marked) {
//        free(current);
//    } else {
//        new = cons(current, new);
//    }
    activeList = new;
}

// Replacement for the C function "exit", that consists of two lines: it calls
// tfree before calling exit. It's useful to have later on; if an error happens,
// you can exit your program, and all memory is automatically cleaned up.
void texit(int status) {
    tfree();
    exit(status);
}

// Marks items in list to not be deleted.
//void mark(Value *value) {
//    value->marked = true;
//}

int getActiveListLength() {
    return length(activeList);
}

