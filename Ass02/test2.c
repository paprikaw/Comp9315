//
// Created by eric white on 1/4/22.
//
#include "reln.h"
#include "chvec.h"
#include <stdio.h>
#include <err.h>
#include "query.h"

int main(int argc, char **argv) {
    // char cv[] = "0,1:0,2:1,1:1,2:2,2";
    char target[] = "1042,child,compact-disc";
    char rname[] = "R";
    char err[MAXERRMSG];  // buffer for error messages
    Reln r;
    Query q;
    char qstr1[] = "1042,?,?"; // 查询成功query
    // char qstr2[] = "121,?,121"; // 查询失败query
    Tuple t;
    // int pid;
    // char tup[MAXTUPLEN];  // buffer for printable tuples

    // Open relation
    if ((r = openRelation(rname,"r+")) == NULL) {
        sprintf(err, "Can't open relation: %s",rname);
        fatal(err);
    }

    if (!existsRelation(rname)) {
        sprintf(err, "No such relation: %s",rname);
        fatal(err);
    }

    // Start query process
    if ((q = startQuery(r, qstr1)) == NULL) {
        sprintf(err, "Invalid query: %s",qstr1);
        fatal(err);
    }

    // execute the query (find matching tuples)
    // 测试查询成功的情况（第一个tuple就成功）
    t = getNextTuple(q);
    assert(strcmp(t, target) == 0);
    printf("test1 success\n");
    free(t);
    closeQuery(q);
    closeRelation(r);

    return 0;
}