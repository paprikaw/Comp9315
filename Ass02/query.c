// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "hash.h"

// A suggestion ... you can change however you like

// get curr candidate hashes from query
Bits getCurrCandidate(Query q, Bits *candidates);
// get page id from a hash based on depth and sp o relation
Bits getPidFromHash(Bits hash, Reln r);
// get candidate hashes from query
Bits *getCandidateHashes(Query q);
// calculate 2^n
int twoToN(int n);
// compare a query to a actual query
int cmpQueryTuple(char* query, Tuple tuple, int nattri);
int numUnknown(Query q);
struct QueryRep {
	Reln    rel;       // need to remember Relation info
	Bits    known;     // the hash value from MAH
	Bits    unknown;   // the unknown bits from MAH
//    int     numUnknown; // Number of unknown bits
    int     cur_candidate; // current index of candidate hash
    Offset  curtup;    // offset of current query within page
    PageID  curpage;   // current page in scan
	int     is_ovflow; // are we in the overflow pages?
    char*   query;     // query string
	//TODO
};

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q)
{
	Query new = malloc(sizeof(struct QueryRep));
    new->known = 0;
    new->unknown = 0;
    new->rel = r;
    assert(new != NULL);

    // Form a query value
    Count nvals = nattrs(r);
    char **vals = malloc(nvals*sizeof(char *));
    assert(vals != NULL);
    tupleVals(q, vals);


    ChVecItem *chVec = chvec(r);
    // If there is no choice factor, raise error
    if (chVec == NULL) {
        printf("No choice vector, please checck");
        return 0;
    }

    // Parse known and unknown Bits
    // Initialize a hash array
    Bits *hashes = malloc(sizeof (hashes) * nattrs(r));
    Bool *is_hashed = malloc(sizeof (Bool) * nattrs(r));
    for (int i = 0; i < nattrs(r); i++) {
        is_hashed[i] = FALSE;
    }

    // 计算unknown的bit的数量
    Count n_unknown = 0;
    for (int i = 0; i < nvals; i++) {
       if (strcmp(vals[i], "?") == 0)  {
           n_unknown++;
       }
    }

    for (int i = 0; i < MAXCHVEC; i++) {
        // 对于每一个choice item遍历，并且将对应的
        Byte attrbute = chVec[i].att;
        Byte bit = chVec[i].bit;
        // 如果bit对应的attribute没有给出的话;
        if (strcmp(vals[attrbute], "?") == 0) {
            new->unknown = setBit(new->unknown, i);
            continue;
        }

        // If there is no hashed value for attribute
        if (is_hashed[attrbute] == FALSE) {
            hashes[attrbute] = hash_any((unsigned char *)vals[attrbute],strlen(vals[attrbute]));
            is_hashed[attrbute] = TRUE;
        }
        // Set bit to 1
        if (bitIsSet(hashes[attrbute], bit)) {
            new->known = setBit(new->known, i);
        }
    }

    Bits *hash_candidate = getCandidateHashes(new);
	// using known bits and first "unknown" value
    new->cur_candidate = 0;
    new->curpage = getPidFromHash(hash_candidate[0], r);
    new->is_ovflow = 0;
    new->curtup = 0;
    new->query = q;
	// set all values in QueryRep object
    free(hashes);
    free(is_hashed);
    free(hash_candidate);
	return new;
}


// get next query during a scan
Tuple getNextTuple(Query q)
{
	// TODO
	// Partial algorithm:
	// if (more tuples in current page)
    // Get rations in query
    // 当现在这个页面不是overflow page的时候, 我们从.data里面去找
    Bits *hash_candidate = getCandidateHashes(q);
    // get next matching query from current page
    Reln r = q->rel;
    char *result = NULL;
    int n_unknown = numUnknown(q);
    // Loop through one candidate page and all its overflow page
    while(1) {
        FILE *file_handle;
        Page p;
        char *data;

        // 判断是否overflow
        if (q->is_ovflow) {
            file_handle = ovflowFile(r);
        } else {
            file_handle = dataFile(r);
        }

        // 得到current page
        p = getPage(file_handle, q->curpage);

        while(1) {
            data = pageData(p);
            for (int i = 0; i < pageNTuples(p); i++) {
                char *curr_tuple = data + q->curtup;
                if (*curr_tuple == '\0') {
                    break;
                }
                // Move the current query pointer to next position
                q->curtup = q->curtup + strlen(curr_tuple) + 1;
                if (cmpQueryTuple(q->query, curr_tuple, nattrs(r))) {
                    result = malloc(strlen(curr_tuple));
                    strcpy(result, curr_tuple);
                    break;
                } else {
                    continue;
                }
            }

            // Found if there is overflow page
            if (result == NULL) {
                // 有overflow page
                if (pageOvflow(p) != NO_PAGE) {
                    if (!q->is_ovflow) {
                        q->is_ovflow = 1;
                        file_handle = ovflowFile(r);
                    }
                    q->curtup = 0;
                    q->curpage = pageOvflow(p);
                    p = getPage(file_handle, pageOvflow(p));
                } else {// 没有overflow page，结束本page的循环
                    break;
                }
            } else { // 找到tuple
                break;
            }
            // 这个这个page没有tuple, 查看是否有overflow page，
        }

        // Find next tuples which is 符合condition
        // If we have found the query
        if (result != NULL) {
            free(hash_candidate);
            free(p);
            return result;
        }

        // 如果还有其它的hash
        if (q->cur_candidate  + 1 < twoToN(n_unknown)) {
            // 换下一个candidate
            q->cur_candidate++;
            q->is_ovflow = 0;
            q->curpage = getPidFromHash(getCurrCandidate(q, hash_candidate), r);
            q->curtup = 0;
        } else { // 没有找到Tuple
            free(hash_candidate);
            free(p);
            return NULL;
        }
    }

    if (q->curtup != PAGESIZE - 1) {
    //    get next matching query from current page
    }
	// else if (current page has overflow)
    else if (q->curtup != PAGESIZE - 1) {
	//    move to overflow page
	//    grab first matching query from page
	// else
	//    move to "next" bucket
	//    grab first matching query from data page
	// endif
	// if (current page has no matching tuples)
	//    go to next page (try again)
	// endif
	return NULL;
    }
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
    free(q);
}


///////////////////// Helper function
// 从Hash中获取page Id, 根据当前relation中的sp
Bits getPidFromHash(Bits hash, Reln r) {
    // compute PageID of first page
    Bits p;
    if (depth(r) == 0)
        p = 0;
    else {
        p = getLower(hash, depth(r));
        if (p < splitp(r)) p = getLower(hash, depth(r)+1);
    }
    return p;
}

Bits getCurrCandidate(Query q, Bits *candidates) {
        return candidates[q->cur_candidate];
}

Bits *getCandidateHashes(Query q) {
    // Construct different situations of hashes:

    Count n_unknown = numUnknown(q);
    Count unknown = q->unknown;
    Count known = q->known;
    Bits *hash_candidate = malloc(sizeof (Bits) * n_unknown);

    // 循环unknown的所有bits，总共循环 2^d次
    for (int i = 0; i < twoToN(n_unknown); i++) {
        Bits curr_num = i;
        Bits generate = 0;
        // 对于每一个情况，我们将一个candidate分配到不同的bit上
        for (int j = 0; j < n_unknown; j++) {
            if (bitIsSet(unknown, j)) {
                // 取得generate的数字的第一个数字
                Bits bit = 1 & curr_num;
                generate = generate | (bit << j);
                curr_num = curr_num >> 1;
            }
        }

        // 获得generate之后的bits
        hash_candidate[i] = known | generate;
    }
    return hash_candidate;
}

int twoToN(int n) {
    return 1 << n;
}

// compare query string to actual query
// e.g.(haha, ?, haha) compare to (haha, niu, haha)
int cmpQueryTuple(char* query, Tuple tuple, int nattri) {
    // Contruct two query array for comparision
    char **queryVal;
    char **tupleVal;
    queryVal = malloc(sizeof (char *) * nattri);
    tupleVal = malloc(sizeof (char *) * nattri);
    tupleVals(query, queryVal);
    tupleVals(tuple, tupleVal);
    int is_equal = 1;
    for (int i = 0; i < nattri; i++)  {
        int is_same = strcmp((queryVal[i]), tupleVal[i]);
        if (strcmp(queryVal[i], "?") == 0 || is_same == 0) {
            continue;
        }
        else {
            is_equal = 0;
            break;
        }
    }
    free(tupleVal);
    free(queryVal);
    return is_equal;
}

// 这个有可能返回0，返回0时有情况：
// 1: 全部都为known的bits
// 当depth为0的时候，只有一个page，故unknown也为0
int numUnknown(Query q) {
    int n_unknown = 0;
    // 如果split pointer不等于0， 那么要求n_depth加一位,用来cover等下query的时候的情况
    int n_depth = (splitp(q->rel) == 0) ? depth(q->rel) : depth(q->rel) + 1;

    int unknown = q->unknown;
    for (int i = 0; i < n_depth; i++) {
        if((unknown >> (n_depth - 1)) & 1) {
            n_unknown++;
        }
    }
    return n_unknown;
}
