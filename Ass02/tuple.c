// query.c ... functions on tuples
// part of Multi-attribute Linear-hashed Files
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "tuple.h"
#include "reln.h"
#include "hash.h"
#include "chvec.h"
#include "bits.h"

// return number of bytes/chars in a query

int tupLength(Tuple t)
{
	return strlen(t);
}

// reads/parses next query in input

Tuple readTuple(Reln r, FILE *in)
{
	char line[MAXTUPLEN];
	if (fgets(line, MAXTUPLEN-1, in) == NULL)
		return NULL;
	line[strlen(line)-1] = '\0';
	// count fields
	// cheap'n'nasty parsing
	char *c; int nf = 1;
	for (c = line; *c != '\0'; c++)
		if (*c == ',') nf++;
	// invalid query
	if (nf != nattrs(r)) return NULL;
    return copyString(line); // needs to be free'd sometime
}

// extract values into an array of strings

void tupleVals(Tuple t, char **vals)
{
	char *c = t, *c0 = t;
	int i = 0;
	for (;;) {
		while (*c != ',' && *c != '\0') c++;
		if (*c == '\0') {
			// end of query; add last field to vals
			vals[i++] = copyString(c0);
			break;
		}
		else if (*c == ','){
			// end of next field; add to vals
			*c = '\0';
			vals[i++] = copyString(c0);
			*c = ',';
			c++; c0 = c;
		}
	}
}

// release memory used for separate attirubte values

void freeVals(char **vals, int nattrs)
{
	int i;
	for (i = 0; i < nattrs; i++) free(vals[i]);
}

// hash a query using the choice vector
// TODO: actually use the choice vector to make the hash

Bits tupleHash(Reln r, Tuple t)
{
	char buf[MAXBITS+1];
	Count nvals = nattrs(r);
    // 分配一个二维数组的内存
	char **vals = malloc(nvals*sizeof(char *));
	assert(vals != NULL);
    // extract values into an array of strings
	tupleVals(t, vals);

    // extract choice vector from ration
    ChVecItem *chVec = chvec(r);
    // If there is no choice factor, raise error
    if (chVec == NULL) {
        printf("No choice vector, please checck");
        return 0;
    }

    // Initialize a hash array
    Bits *hashes = malloc(sizeof (hashes) * nattrs(r));
    Bool *is_hashed = malloc(sizeof (Bool) * nattrs(r));
    for (int i = 0; i < nattrs(r); i++) {
        is_hashed[i] = FALSE;
    }
    Bits hash = 0;
    for (int i = 0; i < MAXCHVEC; i++) {
        // 对于每一个choice item遍历，并且将对应的
        Byte attrbute = chVec[i].att;
        Byte bit = chVec[i].bit;
        if (is_hashed[attrbute] == FALSE) {
            hashes[attrbute] = hash_any((unsigned char *)vals[attrbute],strlen(vals[attrbute]));
            is_hashed[attrbute] = TRUE;
        }
        if (bitIsSet(hashes[attrbute], bit)) {
            hash = setBit(hash, i);
        }
    }
    free(hashes);
    free(is_hashed);
    bitsString(hash, buf);
    printf("hash(%s) = %s\n", t, buf);
    return hash;
}

// compare two tuples (allowing for "unknown" values)

Bool tupleMatch(Reln r, Tuple t1, Tuple t2)
{
	Count na = nattrs(r);
	char **v1 = malloc(na*sizeof(char *));
	tupleVals(t1, v1);
	char **v2 = malloc(na*sizeof(char *));
	tupleVals(t2, v2);
	Bool match = TRUE;
	int i;
	for (i = 0; i < na; i++) {
		// assumes no real attribute values start with '?'
		if (v1[i][0] == '?' || v2[i][0] == '?') continue;
		if (strcmp(v1[i],v2[i]) == 0) continue;
		match = FALSE;
	}
	freeVals(v1,na); freeVals(v2,na);
	return match;
}

// puts printable version of query in user-supplied buffer

void tupleString(Tuple t, char *buf)
{
	strcpy(buf,t);
}
