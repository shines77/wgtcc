// Copyright 2014 Rui Ueyama. Released under the MIT license.
// @wgtcc: passed

#include <iso646.h>
#include "test.h"

#define SS(x) #x
#define S(x) SS(x)

int main() {
    expect_string("&&", S(and));
    expect_string("&=", S(and_eq));
    expect_string("&", S(bitand));
    expect_string("|", S(bitor));
    expect_string("~", S(compl));
    expect_string("!", S(not));
    expect_string("!=", S(not_eq));
    expect_string("||", S(or));
    expect_string("|=", S(or_eq));
    expect_string("^", S(xor));
    expect_string("^=", S(xor_eq));
    return 0;
}
