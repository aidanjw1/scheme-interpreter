#include <stdlib.h>
#include <tokenizer.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "linkedlist.h"
#include "talloc.h"
#include "parser.h"
#include "interpreter.h"


void test1() {
    TEST_ASSERT_EQUAL_INT(1,1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test1);
    texit(0);
    return UNITY_END();
}
