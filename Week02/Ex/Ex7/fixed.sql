create table R (
	x integer,
	c char(10),
	y float,
--	d varchar(10), // ruins fixed-size
	primary key(x)
);

without d, size if fixed 24 bytes
with d, size varies from 28 to 38 bytes
