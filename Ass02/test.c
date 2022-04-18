//
// Created by eric white on 1/4/22.
//
#include "reln.h"
#include "chvec.h"
#include <stdio.h>
#include <err.h>
#include "query.h"

int main(int argc, char **argv) {
    char cv[] = "0,0:1,0:2,0:0,1:1,1:2,1";
    char test_tuple[] = "121,22,123";
    char rname[] = "test";
    char err[MAXERRMSG];  // buffer for error messages
    Reln r;
    Query q;
    char qstr1[] = "121,?,123"; // 查询成功query
    char qstr2[] = "121,?,121"; // 查询失败query
    Tuple t;
    int pid;
    char tup[MAXTUPLEN];  // buffer for printable tuples

    // Create relation
    if (newRelation(rname, 3, 4, 0,cv) != 0) {
        printf ("error happened");
        return 1;
    }


    // Open relation
    if ((r = openRelation(rname,"r+")) == NULL) {
        sprintf(err, "Can't open relation: %s",rname);
        fatal(err);
    }

    if (!existsRelation(rname)) {
        sprintf(err, "No such relation: %s",rname);
        fatal(err);
    }

    // insert a new tuple
    pid = addToRelation(r,test_tuple);
    tupleString(test_tuple,tup); // printable version
    if (pid == NO_PAGE) {
        sprintf(err, "Insert of %s failed\n", tup);
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
    assert(strcmp(t, test_tuple) == 0);
    printf("test1 success\n");
    free(t);
    closeQuery(q);

    // 测试查询失败的情况

    if ((q = startQuery(r, qstr2)) == NULL) {
        sprintf(err, "Invalid query: %s",qstr1);
        fatal(err);
    }
    t = getNextTuple(q);
    assert(t == NULL);
    printf("test2 success\n");
    free(t);
    closeQuery(q);

    // clean up

    closeRelation(r);

    return 0;
}