// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "hash.h"
#include "util.h"
// A suggestion ... you can change however you like

// get curr candidate hashes from query
Bits getCurrCandidate(Query q, Bits *candidates);
// get page id from a hash based on depth and sp o relation
Bits getPidFromHash(Bits hash, Reln r);
// get candidate hashes from query
Bits *getCandidateHashes(Query q);
// compare a query to a actual query
int cmpQueryTuple(char* query, Tuple tuple, int nattri);
int numUnknown(Query q);
int twoToN(int n);
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
    Bits *hashes = malloc(sizeof (Bits) * nattrs(r));
    int *is_hashed = malloc(sizeof (int) * nattrs(r));
    for (int i = 0; i < nattrs(r); i++) {
        is_hashed[i] = 0;
    }

    for (int i = 0; i < MAXCHVEC; i++) {
        // ???????????????choice item???????????????????????????
        Byte attrbute = chVec[i].att;
        Byte bit = chVec[i].bit;
        // ??????bit?????????attribute??????????????????;
        if (strcmp(vals[attrbute], "?") == 0) {
            new->unknown = setBit(new->unknown, i);
            continue;
        }

        // If there is no hashed value for attribute
        if (is_hashed[attrbute] == 0) {
            hashes[attrbute] = hash_any((unsigned char *)vals[attrbute],strlen(vals[attrbute]));
            is_hashed[attrbute] = 1;
        }
        // Set bit to 1
        if (bitIsSet(hashes[attrbute], bit)) {
            new->known = setBit(new->known, i);
        }
    }

    Bits *hash_candidate = getCandidateHashes(new);
	// set all values in QueryRep object
    new->cur_candidate = 0;
    new->curpage = getPidFromHash(hash_candidate[0], r);
    new->is_ovflow = 0;
    new->curtup = 0;
    new->query = q;
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
    // ???????????????????????????overflow page?????????, ?????????.data????????????
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

        // ????????????overflow
        if (q->is_ovflow) {
            file_handle = ovflowFile(r);
        } else {
            file_handle = dataFile(r);
        }

        // ??????current page
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
                // ???overflow page
                if (pageOvflow(p) != NO_PAGE) {
                    if (!q->is_ovflow) {
                        q->is_ovflow = 1;
                        file_handle = dataFile(r);
                    }

                    // ????????????page??????disk
                    int newPage = pageOvflow(p);
                    free(p);
                    q->curpage = newPage;
                    q->curtup = 0;
                    p = getPage(ovflowFile(r), pageOvflow(p));
                } else {// ??????overflow page????????????page?????????
                    break;
                }
            } else { // ??????tuple
                break;
            }
            // ????????????page??????tuple, ???????????????overflow page???
        }

        // Find next tuples which is ??????condition
        // If we have found the query
        if (result != NULL) {
            free(hash_candidate);
            free(p);
            return result;
        }

        // ????????????????????????????????????hash
        while (q->cur_candidate < twoToN(n_unknown)) {
            q->cur_candidate++;
            PageID cur_pid = getPidFromHash(getCurrCandidate(q, hash_candidate), r);
            if (cur_pid < twoToN(depth(r)) + splitp(r)) {
                q->is_ovflow = 0;
                q->curpage = cur_pid;
                q->curtup = 0;
                break;
            }
        }

        // ???????????????????????????hash??????????????????tuple?????????NULL
        if (q->cur_candidate >= twoToN(n_unknown)) {
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
// ???Hash?????????page Id, ????????????relation??????sp
Bits getPidFromHash(Bits hash, Reln r) {
    // compute PageID of first page
    Bits p;
    if (depth(r) == 0)
        p = 0;
    else {
        if (splitp(r) != 0) {
            p = getLower(hash, depth(r) + 1);
        } else {
            p = getLower(hash, depth(r));
        }
        // p = getLower(hash, depth(r));
        // if (p < splitp(r)) p = getLower(hash, depth(r)+1);
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
    Bits *hash_candidate = malloc(sizeof (Bits) * twoToN(n_unknown));

    // ??????unknown?????????bits??????????????? 2^d???
    for (int i = 0; i < twoToN(n_unknown); i++) {
        Bits curr_num = i;
        Bits generate = 0;
        // ???????????????????????????????????????candidate??????????????????bit???
        int my_depth = splitp(q->rel) == 0 ? depth(q->rel) : depth(q->rel) + 1;
        for (int j = 0; j < my_depth; j++) {
            if (bitIsSet(unknown, j)) {
                // ??????generate???????????????????????????
                Bits bit = 1 & curr_num;
                generate = generate | (bit << j);
                curr_num = curr_num >> 1;
            }
        }

        // ??????generate?????????bits
        hash_candidate[i] = known | generate;
    }
    return hash_candidate;
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
    freeVals(tupleVal, nattri);
    freeVals(queryVal, nattri);
    return is_equal;
}

// ?????????????????????0?????????0???????????????
// 1: ????????????known???bits
// ???depth???0????????????????????????page??????unknown??????0
int numUnknown(Query q) {
    int n_unknown = 0;
    // ??????split pointer?????????0??? ????????????n_depth?????????,??????cover??????query??????????????????
    int n_depth = (splitp(q->rel) == 0) ? depth(q->rel) : (depth(q->rel) + 1);
    int unknown = q->unknown;
    for (int i = 0; i < n_depth; i++) {
        if((unknown >> i) & 1) {
            n_unknown++;
        }
    }
    return n_unknown;
}

int twoToN(int n) {
    return 1 << n;
}