# Catalog

Consider what information the RDBMS needs about relations:

* name, owner, primary key of each relation

* name, data type, constraints for each attribute
* authorisation for operations on each relation 

Similarly for other DBMS objects (e.g. views, functions, triggers, ...)

This information is stored in the system catalog tables

Standard for catalogs in SQL:2003: INFORMATION_SCHEMA 

在pgsql的实现中，所有的catalog表都位于catalog的namespace中

# 具体细节

## atributes

attribtue的id一般是正的，当然也有一些额外的attributeId是负的，所以我们在使用catalog table获取attribute时需要考虑到这一点

一下代码片段对attnum判断了正负:
``` sql
select t.relname, a.attname, a.attnum
   from   pg_class t
       join pg_attribute a on (a.attrelid=t.oid)
       join pg_namespace n on (t.relnamespace=n.oid)
   where  t.relkind='r' and n.nspname = 'public'
       and a.attnum > 0
```

[具体代码](../Week01/Ex/Ex4/schema.sql)