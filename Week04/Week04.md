# Question 1:

Consider performing a nested-loop join on two tables as follows:

``` C
for i in 0..nPages(R)-1 { 
   read page i from R 
   for j in 0..nPages(S)-1 { 
      read page j from S 
      check all pairs of tuples in R page and S page 
      add those satisfying join condition to output
   }
} 

```

If table R contains 500 large tuples stored in 10 pages and table S contains 1000 small tuples stored in 5 pages, and there is one input buffer for each table and one output buffer to hold result tuples, what is the total number of page reads that will occur in executing the above query method?

Table R: 500 Tuples, 10 Pages, 1 Input buffer

Table S: 1000 Tuples, 5 pages, 1 Input buffer

Totally 1 output tuple

Inner loop reading: 5 * 10
Outter loop reading: 10

Totally: 50 + 10 = 60

# Question 2

Consider two tables R and S, like the tables in Q1, where nPages(R) = 10 and nPages(S) = 5.

Now consider the execution of a nested-loop on these two tables, this time using a buffer pool:

``` 
for i in 0..nPages(R)-1 { 
   request page i from R
   for j in 0..nPages(S)-1 { 
      request page j from S 
      check all pairs of tuples in R page and S page
      add those satisfying join condition to output
      release page j from S
   }
   release page i from R
}

```

The buffer pool contains 7 buffers and is initially empty. One buffer will be locked into memory for use as an output buffer, while the other 6 buffers will be available to the buffer-pool manager to handle page requests. Assuming an MRU buffer replacement strategy, where most-recent is determined by "last released", how many pages will be read in executing the above nested-loop join? Note: we are asking for the number of pages actually read from disk, not simply the number of page requests.

Table R: 500 Tuples, 10 Pages, 1 Input buffer

Table S: 1000 Tuples, 5 pages, 1 Input buffer

7 buffers pool: 6 input buffer, 1 output buffer

Most Recently Used (MRU) 

