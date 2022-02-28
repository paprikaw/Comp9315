// Compacted free space
// Assume the following definitions

typedef ... Byte;    // one-byte value
typedef ... Count;   // two-byte value
typedef ... Offset;  // two-byte value

#define PAGESIZE ...

typedef struct {
    Offset  freeTop;
    Count   nrecs;
    Offset  rec[1];  // start of directory
                     // data space 
} Page;

// Initialise Page data

void initPage(Page *p)
{
    p->nrecs = 0;
    p->freeTop = PAGESIZE-1;
    Byte *data;
    data = &(p->rec[0]);
    int nBytes = PAGESIZE - 2*sizeof(Byte2);
    for (int i = 0; i < nBytes; i++) data[i] = 0;
}

// Add record into Page; return record index

int insert(Page *p, Record *r)
{
    Count rs = recSize(r);
	Count directorySize = p->nrecs*sizeof(Offset);
	Count freeSpace = p->freeTop - (2*sizeof(Byte2)+directorySize);
	if (rs > freeSpace-sizeof(Count)) Page Is Full
	Offset location = p->freeTop - rs;
	memcpy(&p[location], r, rs);
	Count index = p->nrecs++;
	p->rec[p->nrecs] = location;
	return index;
}
