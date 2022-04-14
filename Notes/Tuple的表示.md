# Tuple 的表示

# Tuple Vs Record

![picture 1](images/0def861c72f20c97205739dc20162a7af1d2999f92049a48a6905084771c8e86.png)  

所以数据库想要实现tuple的访问，需要经历

    Record -> Inteprete as tuple -> access by tuple

## 互相的转化
 A Record is an array of bytes (byte[])

    representing the data values from a typed Tuple
    stored on disk (persistent) or in a memory buffer 

A Tuple is a collection of named,typed values  (cf. C struct)

    to manipulate the values, need an "interpretable" structure
    stored in working memory, and temporary 

![picture 2](images/6ae51a71c0cade54b79a2b6389d3a74b2dd6f658187f8e86905f70ec0b66d71b.png)

具体步骤

1. retrive record

通过表的id和record的id取得record
``` c
Record get_record(Relation rel, RecordId rid) {
    (pid,tid) = rid;
    Page buf = get_page(rel, pid);
    return get_bytes(rel, buf, tid);
}
```
2. 将record转化为tuple

``` c
Relation rel = ... // relation schema
Record rec = get_record(rel, rid)
Tuple t = mkTuple(rel, rec)
```
table的schema数据可以帮我们搞清楚table的数据类型有什么

record中特殊的结构可以帮助我们搞清楚variable-length record中properties是怎么安放的

Record不同的结构:


![picture 3](images/a5cc394038040239ef447f5bb71f316da836a17f96b1e294c4fc639bebaf7019.png)  


3. tuple的表示

    ![picture 4](images/47aa36d2ba58ca988321c66c0de006527e0d848d281df992d276fb916b31c37d.png)  


1. 由tuple的 field descriptor关联到具体的数据

    两种方式M

    在field descriptor中使用pointer指向最终的数据

    ![picture 5](images/84c1016178f22fbb2e1a2ed2a7ef92d4d14a1be8103c09afb067d68d60431825.png)  

    直接把数据append到field descriptor的后面

    ![picture 6](images/ba7993a23b588616179e054e30895f9e444b7b159f33259c31a1dcc82c8c9b74.png)  


