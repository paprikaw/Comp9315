这篇笔记会伴随一个assignment的实践。

# 为什么需要一个新的datatype？

# 步骤


1. Creating a new base type requires

    * telling the SQL front-end about it
    * building C functions to manipulate values of the type
    * setting up ordering to allow indexing 

2. At the SQL level (pname.source)...
``` sql
create type PersonName ( type info and function links )
```

3. Also useful to define comparison operators on the type   (e.g. < > = ) 
