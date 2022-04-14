# Query处理流程
1. 将sql转化为RA
2. build query plan（RaOps)
3. 评估cost
4. 选择一个cost最少的query
注意：query选择器不可能遍历一个所有query play O(n^2)

# Query proccessing


