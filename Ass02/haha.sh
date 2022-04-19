#!/bin/dash
rm -r R.*
make clean
make
./create R 3 2 "0,0:1,0:2,0:0,1:1,1:2,1"
./insert R < ./data1.txt
# rm xyz.*
# echo ""
# echo ""
# echo "Test 15: Create a new MALH file 3 attrs, 4 pages"
# echo ""
# ./create xyz 3 4 "" | head -10
# echo "... etc etc"
# echo ""
# echo "Stats on new file"
# echo ""
# ./stats xyz

# rm xyz.*
# echo ""
# echo ""
# echo "Test 17: Create a new MALH file 5 attrs, 16 pages"
# echo ""
# ./create xyz 5 16 "" | head -10
# echo "... etc etc"
# echo ""
# echo "Stats on new file"
# echo ""
# ./stats xyz
# echo ""
# echo ""
# echo "Test 18: Insert 10000 tuples"
# ./insert xyz < ./test-files/data5.txt > /dev/null
# echo ""
# echo "Stats after inserting tuples"
# echo "Should be ntuples=10000, npages= ~500, sp=?"
# echo "Only showing first 10 pages and last 5 pages"
# echo ""
# ./stats xyz | head -16
# echo "....."
# ./stats xyz | tail -5
# echo ""
# ls -l xyz.*
