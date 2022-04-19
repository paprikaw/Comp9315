// reln.c ... functions on Relations
// part of Multi-attribute Linear-hashed Files
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "reln.h"
#include "page.h"
#include "tuple.h"
#include "chvec.h"
#include "bits.h"
#include "hash.h"
#include "util.h"

#define HEADERSIZE (3*sizeof(Count)+sizeof(Offset))

void splitPage (Reln r);
// void splitPage ();
Bits getPidFromHashByDepth(Bits hash, int depth);
int twoToNv1(int n);
struct RelnRep {
	Count  nattrs; // number of attributes
	Count  depth;  // depth of main data file
	Offset sp;     // split pointer
    Count  npages; // number of main data pages
    Count  ntups;  // total number of tuples
	ChVec  cv;     // choice vector
	char   mode;   // open for read/write
	FILE  *info;   // handle on info file
	FILE  *data;   // handle on data file
	FILE  *ovflow; // handle on ovflow file
};

// create a new relation (three files)

Status newRelation(char *name, Count nattrs, Count npages, Count d, char *cv)
{
    char fname[MAXFILENAME];
	Reln r = malloc(sizeof(struct RelnRep));
	r->nattrs = nattrs; r->depth = d; r->sp = 0;
	r->npages = npages; r->ntups = 0; r->mode = 'w';
	assert(r != NULL);
	if (parseChVec(r, cv, r->cv) != OK) return ~OK;
	sprintf(fname,"%s.info",name);
	r->info = fopen(fname,"w");
	assert(r->info != NULL);
	sprintf(fname,"%s.data",name);
	r->data = fopen(fname,"w");
	assert(r->data != NULL);
	sprintf(fname,"%s.ovflow",name);
	r->ovflow = fopen(fname,"w");
	assert(r->ovflow != NULL);
	int i;
	for (i = 0; i < npages; i++) addPage(r->data);
	closeRelation(r);
	return 0;
}

// check whether a relation already exists

Bool existsRelation(char *name)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.info",name);
	FILE *f = fopen(fname,"r");
	if (f == NULL)
		return FALSE;
	else {
		fclose(f);
		return TRUE;
	}
}

// set up a relation descriptor from relation name
// open files, reads information from rel.info

Reln openRelation(char *name, char *mode)
{
	Reln r;
	r = malloc(sizeof(struct RelnRep));
	assert(r != NULL);
	char fname[MAXFILENAME];
	sprintf(fname,"%s.info",name);
	r->info = fopen(fname,mode);
	assert(r->info != NULL);
	sprintf(fname,"%s.data",name);
	r->data = fopen(fname,mode);
	assert(r->data != NULL);
	sprintf(fname,"%s.ovflow",name);
	r->ovflow = fopen(fname,mode);
	assert(r->ovflow != NULL);
	// Naughty: assumes Count and Offset are the same size
	int n = fread(r, sizeof(Count), 5, r->info);
	assert(n == 5);
	n = fread(r->cv, sizeof(ChVecItem), MAXCHVEC, r->info);
	assert(n == MAXCHVEC);
	r->mode = (mode[0] == 'w' || mode[1] =='+') ? 'w' : 'r';
	return r;
}

// release files and descriptor for an open relation
// copy latest information to .info file

void closeRelation(Reln r)
{
	// make sure updated global data is put in info
	// Naughty: assumes Count and Offset are the same size
	if (r->mode == 'w') {
		fseek(r->info, 0, SEEK_SET);
		// write out core relation info (#attr,#pages,d,sp)
		int n = fwrite(r, sizeof(Count), 5, r->info);
		assert(n == 5);
		// write out choice vector
		n = fwrite(r->cv, sizeof(ChVecItem), MAXCHVEC, r->info);
		assert(n == MAXCHVEC);
	}
	fclose(r->info);
	fclose(r->data);
	fclose(r->ovflow);
	free(r);
}

// insert a new query into a relation
// returns index of bucket where inserted
// - index always refers to a primary data page
// - the actual insertion page may be either a data page or an overflow page
// returns NO_PAGE if insert fails completely
// TODO: include splitting and file expansion
PageID addToRelation(Reln r, Tuple t)
{
      // 在每次insertion之前，首先判断是否需要split
      int C = 1024 / (10 * r->nattrs);

      // tuple数量已经达到C值，需要split
      if (((r->ntups + 1) % C)  == 0) {
          splitPage(r);
          //split pointer 到达 2 ^ d
           if (r->sp == twoToNv1(r->depth) - 1) {
                r->depth++;
                 r->sp = 0;
           } else {
                 r->sp++;
           }
      }


	Bits h, p;
	// char buf[MAXBITS+1];
	h = tupleHash(r,t);
	if (r->depth == 0)
		p = 0;
	else {
		p = getLower(h, r->depth);
		if (p < r->sp) p = getLower(h, r->depth+1);
	}
	// bitsString(h,buf); printf("hash = %s\n",buf);
	// bitsString(p,buf); printf("page = %s\n",buf);
	Page pg = getPage(r->data,p);
	if (addToPage(pg,t) == OK) {
		putPage(r->data,p,pg);
		r->ntups++;
		return p;
	}
	// primary data page full
	if (pageOvflow(pg) == NO_PAGE) {
		// add first overflow page in chain
		PageID newp = addPage(r->ovflow);
		pageSetOvflow(pg,newp);
		putPage(r->data,p,pg);
		Page newpg = getPage(r->ovflow,newp);
		// can't add to a new page; we have a problem
		if (addToPage(newpg,t) != OK) return NO_PAGE;
		putPage(r->ovflow,newp,newpg);
		r->ntups++;
		return p;
	}
	else {
		// scan overflow chain until we find space
		// worst case: add new ovflow page at end of chain
		Page ovpg, prevpg = NULL;
		PageID ovp, prevp = NO_PAGE;
		ovp = pageOvflow(pg);
		while (ovp != NO_PAGE) {
			ovpg = getPage(r->ovflow, ovp);
			if (addToPage(ovpg,t) != OK) {
				prevp = ovp; prevpg = ovpg;
				ovp = pageOvflow(ovpg);
			}
			else {
				if (prevpg != NULL) free(prevpg);
				putPage(r->ovflow,ovp,ovpg);
				r->ntups++;
				return p;
			}
		}
		// all overflow pages are full; add another to chain
		// at this point, there *must* be a prevpg
		assert(prevpg != NULL);
		// make new ovflow page
		PageID newp = addPage(r->ovflow);
		// insert query into new page
		Page newpg = getPage(r->ovflow,newp);
        if (addToPage(newpg,t) != OK) return NO_PAGE;
        putPage(r->ovflow,newp,newpg);
		// link to existing overflow chain
		pageSetOvflow(prevpg,newp);
		putPage(r->ovflow,prevp,prevpg);
        r->ntups++;
		return p;
	}
	return NO_PAGE;
}

// external interfaces for Reln data

FILE *dataFile(Reln r) { return r->data; }
FILE *ovflowFile(Reln r) { return r->ovflow; }
Count nattrs(Reln r) { return r->nattrs; }
Count npages(Reln r) { return r->npages; }
Count ntuples(Reln r) { return r->ntups; }
Count depth(Reln r)  { return r->depth; }
Count splitp(Reln r) { return r->sp; }
ChVecItem *chvec(Reln r)  { return r->cv; }


// displays info about open Reln

void relationStats(Reln r)
{
	printf("Global Info:\n");
	printf("#attrs:%d  #pages:%d  #tuples:%d  d:%d  sp:%d\n",
	       r->nattrs, r->npages, r->ntups, r->depth, r->sp);
	printf("Choice vector\n");
	printChVec(r->cv);
	printf("Bucket Info:\n");
	printf("%-4s %s\n","#","Info on pages in bucket");
	printf("%-4s %s\n","","(pageID,#tuples,freebytes,ovflow)");
	for (Offset pid = 0; pid < r->npages; pid++) {
		printf("[%2d]  ",pid);
		Page p = getPage(r->data, pid);
		Count ntups = pageNTuples(p);
		Count space = pageFreeSpace(p);
		Offset ovid = pageOvflow(p);
		printf("(d%d,%d,%d,%d)",pid,ntups,space,ovid);
		free(p);
		while (ovid != NO_PAGE) {
			Offset curid = ovid;
			p = getPage(r->ovflow, ovid);
			ntups = pageNTuples(p);
			space = pageFreeSpace(p);
			ovid = pageOvflow(p);
			printf(" -> (ov%d,%d,%d,%d)",curid,ntups,space,ovid);
			free(p);
		}
		putchar('\n');
	}
}

void splitPage (Reln r) {

     // left 和 right 的page ID
      PageID new_pid = twoToNv1(r->depth) + r->sp;
      PageID old_pid = r->sp;

      int cur_pos = 0;
      Page old_page;
      Page left_page;
      Page right_page;

      int left_page_id = old_pid;
      int right_page_id = new_pid;

      int is_left_ov = 0;
      int is_right_ov = 0;
      int is_old_ov = 0;

      old_page = getPage(dataFile(r), old_pid);

      left_page = newPage();
      if (pageOvflow(old_page) != NO_PAGE) {
          pageSetOvflow(left_page, pageOvflow(old_page));
      }
      right_page = newPage();
      // 遍历所有的old page
      for(;;) {
          char *data = pageData(old_page);
         for (int i = 0; i < pageNTuples(old_page); i++) {
             char *curr_tuple = data + cur_pos;
             if (*curr_tuple == '\0') {
                 break;
             }
             // Move the current query pointer to next position
             cur_pos += strlen(curr_tuple) + 1;
             // 对于当前的tuple，计算其hash
             Bits hash = tupleHash(r, curr_tuple);
             int new_page_id = getPidFromHashByDepth(hash, r->depth + 1);
             // 如果新的hash指向左边的page
             if (new_page_id == old_pid) {
                 if (addToPage(left_page,curr_tuple) == OK) {
                     continue;
                 }
                 // 如果左边tuple的buffer已经满了
                 // tuple buffer不是overflow page
                 FILE *leftFileHandler;
                 if (!is_left_ov) {
                     is_left_ov = 1;
                     leftFileHandler = dataFile(r);
                     // 将tuple插入到回buffer
                      // is_left_ov = 1;
                      // putPage(dataFile(r), left_page_id, left_page);
                      // PageID newp = addPage(dataFile(r));
                 } else {
                     leftFileHandler = ovflowFile(r);
                     // tuple buffer是overflow page, 将left buffer放回tuple buffer
                     // putPage(ovflowFile(r), left_page_id, left_page);
                     // PageID newp = addPage(ovflowFile(r));
                 }

                 if (pageOvflow(left_page) != NO_PAGE)  {
                     putPage(leftFileHandler, left_page_id, left_page);
                     left_page_id = pageOvflow(left_page);
                     left_page = getPage(leftFileHandler, left_page_id);
                 } else {
                 PageID newp = addPage(r->ovflow);
                 pageSetOvflow(left_page, newp);
                 putPage(leftFileHandler, left_page_id, left_page);
                 left_page_id = newp;
                 left_page = getPage(ovflowFile(r), left_page_id);
                 }
                 assert(addToPage(left_page, curr_tuple) == OK);
                 // addToPage(left_page, curr_tuple);
             } else {
                 if (addToPage(right_page,curr_tuple) == OK) {
                     continue;
                 }
                 // 如果右边tuple的buffer已经满了
                 // tuple buffer不是overflow page
                 FILE *rightFileHandler;
                 if (!is_right_ov) {
                     is_right_ov = 1;
                     rightFileHandler = dataFile(r);
                     // 将tuple插入到回buffer
                     // is_left_ov = 1;
                     // putPage(dataFile(r), left_page_id, left_page);
                     // PageID newp = addPage(dataFile(r));
                 } else {
                     rightFileHandler = ovflowFile(r);
                     // tuple buffer是overflow page, 将left buffer放回tuple buffer
                     // putPage(ovflowFile(r), left_page_id, left_page);
                     // PageID newp = addPage(ovflowFile(r));
                 }

                 if (pageOvflow(right_page) != NO_PAGE)  {
                     putPage(rightFileHandler, right_page_id, right_page);
                     right_page_id = pageOvflow(right_page);
                     right_page = getPage(rightFileHandler, right_page_id);
                 } else {
                    PageID newp = addPage(r->ovflow);
                    pageSetOvflow(right_page, newp);
                    putPage(rightFileHandler, right_page_id, right_page);
                    right_page_id = newp;
                    right_page = getPage(ovflowFile(r), right_page_id);
                 }
                 assert(addToPage(right_page, curr_tuple) == OK);
                 // addToPage(right_page, curr_tuple);
                 // if (!is_right_ov) {
                 //     // 将tuple插入到回buffer
                 //     is_right_ov = 1;
                 //     putPage(dataFile(r), right_page_id, right_page);
                 // } else {
                 //     // tuple buffer是overflow page, 将left buffer放回tuple buffer
                 //     putPage(ovflowFile(r), right_page_id, right_page);
                 // }
                 // if (pageOvflow(right_page) != NO_PAGE)  {
                 //     right_page_id = pageOvflow(right_page);
                 //     right_page = getPage(ovflowFile(r), right_page_id);
                 // } else {
                 //     right_page_id = addPage(ovflowFile(r));
                 //     right_page = getPage(ovflowFile(r), right_page_id);
                 // }
                 // assert(addToPage(right_page, curr_tuple) == OK);
             }
         }
         if (pageOvflow(old_page) != NO_PAGE) {
             int ov_pid = pageOvflow(old_page);
             Page emptyPage = newPage();
             if (is_old_ov) {
                 putPage(ovflowFile(r), old_pid, emptyPage);
             } else {
                 is_old_ov = 1;
                 putPage(dataFile(r), old_pid, emptyPage);
             }

             old_page = getPage(ovflowFile(r), ov_pid);
             old_pid = ov_pid;
             cur_pos = 0;
         } else {
             // 结束循环，将对应的left page和right page放回dis
             Page emptyPage = newPage();
             if (is_old_ov) {
                 putPage(ovflowFile(r), old_pid, emptyPage);
             } else {
                 putPage(dataFile(r), old_pid, emptyPage);
             }

             putPage(is_right_ov ? ovflowFile(r) : dataFile(r), right_page_id, right_page);
             putPage(is_left_ov ? ovflowFile(r) : dataFile(r), left_page_id, left_page);
             // 处理empty的overflow page
             // if(pageOvflow(left_page) != NO_PAGE) {
             //     Page emptyPage = newPage();
             //     putPage(ovflowFile(r), pageOvflow(left_page), emptyPage);
             // }
             // if(pageOvflow(right_page) != NO_PAGE) {
             //     Page emptyPage = newPage();
             //     putPage(ovflowFile(r), pageOvflow(right_page), emptyPage);
             // }
             r->npages++;
             break;
         }
     }
 }

Bits getPidFromHashByDepth(Bits hash, int depth) {
     // compute PageID of first page
     PageID p = getLower(hash, depth);
     return p;
}
int twoToNv1(int n) {
    return 1 << n;
}
