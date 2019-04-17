#!/bin/bash

cd `dirname $0`

try() {
	expected="$1"
	input="$2"

	../bin/shcc "$input" > out.s
	gcc -o out out.s exfunc.o
	./out
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$expected expected, but got $actual"
		exit 1
	fi
}

try 0 '0;'
try 42 '42;'

try 42 '40+2;'
try 21 '5+20-4;'
try 0 '0+0-0;'

try 21 '5 + 20 - 4;'
try 42 '10+ 10 +22;'

try 15 '3*5;'
try 20 '60/3;'
try 22 '4+6*3;'
try 22 '6*3+4;'
try 12 '(2+1)*4;'
try 12 '4*(2+1);'

try 47 '5+6*7;'
try 15 '5*(9-6);'
try 4 '(3+5)/2;'

try 10 'a = 10;'
try 22 'a=10; b=12 ;c=a+b;'
try 128 'a=20;a=10;z=12;y=30;x=22;z*a+y-x;'

try 128 'a=20;a=10;z=12;y=30;x=22;return z*a+y-x;'
try 12 'a=20;a=10;z=12;y=30;x=22;z*a+y-x; return z;'
try 20 'a=20;return a;a=10;z=12;y=30;x=22;z*a+y-x;'
try 20 'a=20;return a;a=10;return a;z=12;y=30;x=22;z*a+y-x;'

try 10 'abc = 10; return abc;'
try 10 'zyx = 10; return zyx;'
try 10 'foobar = 10; return foobar;'
try 10 'a123 = 10; return a123;'
try 10 'a1=2; a2=3; a3=4; a4=5; a5=6; a6=7; a7=8; a8=9; a9=10; a10=11; a11=12; a12=13; a13=14; a14=15; a15=16; a16=17; a17=18; a18=19; a19=20; a20=21; a21=22; a22=23; a23=24; a24=25; a25=26; a26=27; a27=28; a28=29; a29=30; a30=31; return a9;'
try 0 'return 1==2;'
try 1 'return 1==1;'
try 0 'return 10!=10;'
try 1 'return 10!=20;'
try 1 'foo=10==10; return foo;'
try 0 'foo=10!=10; return foo;'

try 42 'exfunc1();return 42;'
try 21 'exfunc2(42);return 21;'
try 42 'exfunc2(1+2);return 42;'
try 42 'exfunc3(42 21);return 42;'
try 21 'exfunc3(21+21 15+6);return 21;'
try 42 'a=21;b=42;exfunc3(a b);return 42;'
try 21 'a=21;b=42;exfunc3(a+b b);return 21;'

echo OK

