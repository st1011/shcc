#!/bin/bash

cd `dirname $0`

# テスト用の外部ファイルのコンパイルが未実行なら実行
if [ ! -e "exfunc.o" ]; then
	gcc -c "exfunc.c"
fi

# テスト用のメソッド
# 第2引数をソースコードとしてコンパイラへ入力・実行し第1引数の予測結果と比較します。
# Arg 1: 予想される返却値
# Arg 2: ソースコード
try() {
	expected="$1"
	input="$2"

	../bin/shcc "$input" > testout.s
	gcc -g -o testout testout.s exfunc.o
	./testout
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$expected expected, but got $actual (L$BASH_LINENO)"
		exit 1
	fi
}

try 0 'main(){0;}'
try 42 'main(){42;}'

try 42 'main(){40+2;}'
try 21 'main(){5+20-4;}'
try 0 'main(){0+0-0;}'

try 21 'main(){5 + 20 - 4;}'
try 42 'main(){10+ 10 +22;}'

try 15 'main(){3*5;}'
try 20 'main(){60/3;}'
try 22 'main(){4+6*3;}'
try 22 'main(){6*3+4;}'
try 12 'main(){(2+1)*4;}'
try 12 'main(){4*(2+1);}'

try 47 'main(){5+6*7;}'
try 15 'main(){5*(9-6);}'
try 4 'main(){(3+5)/2;}'

try 10 'main(){a = 10;}'
try 22 'main(){a=10; b=12 ;c=a+b;}'
try 22 'main(){a=20; a=10; b=12 ;c=a+b;}'
try 128 'main(){a=20;a=10;z=12;y=30;x=22;z*a+y-x;}'

try 128 'main(){a=20;a=10;z=12;y=30;x=22;return z*a+y-x;}'
try 12 'main(){a=20;a=10;z=12;y=30;x=22;z*a+y-x; return z;}'
try 20 'main(){a=20;return a;a=10;z=12;y=30;x=22;z*a+y-x;}'
try 20 'main(){a=20;return a;a=10;return a;z=12;y=30;x=22;z*a+y-x;}'

try 10 'main(){abc = 10; return abc;}'
try 10 'main(){zyx = 10; return zyx;}'
try 10 'main(){foobar = 10; return foobar;}'
try 10 'main(){a123 = 10; return a123;}'
try 10 'main(){a1=2; a2=3; a3=4; a4=5; a5=6; a6=7; a7=8; a8=9; a9=10; a10=11; a11=12; a12=13; a13=14; a14=15; a15=16; a16=17; a17=18; a18=19; a19=20; a20=21; a21=22; a22=23; a23=24; a24=25; a25=26; a26=27; a27=28; a28=29; a29=30; a30=31; return a9;}'
try 0 'main(){return 1==2;}'
try 1 'main(){return 1==1;}'
try 0 'main(){return 10!=10;}'
try 1 'main(){return 10!=20;}'
try 1 'main(){foo=10==10; return foo;}'
try 0 'main(){foo=10!=10; return foo;}'

try 42 'main(){exfunc1();return 42;}'
try 21 'main(){exfunc2(42);return 21;}'
try 42 'main(){exfunc2(1+2);return 42;}'
try 42 'main(){exfunc3(42 21);return 42;}'
try 21 'main(){exfunc3(21+21 15+6);return 21;}'
try 42 'main(){a=21;b=42;exfunc3(a b);return 42;}'
try 21 'main(){a=21;b=42;exfunc3(a+b b);return 21;}'

try 42 'main(){return exfunc4();}'
try 42 'main(){a = exfunc4();return a;}'
try 42 'main(){a = exfunc4();b=exfunc4();return a;}'
try 42 'main(){a = exfunc4();b=exfunc4();return b;}'

try 42 'main(){return exfunc5(40 2);}'
try 21 'main(){a = 11;b = 10; return exfunc5(a b);}'
try 42 'main(){return exfunc5(20+20 1+1);}'
try 42 'main(){a = exfunc5(20 1);b = 21; return a + b;}'
try 42 'main(){a = exfunc5(20 1);b = exfunc5(20 1); c = exfunc5(a b); return c;}'
try 42 'main(){a = exfunc5(20 1);b = exfunc5(20 1); return c = exfunc5(a b);}'

try 0 'main(){return 20 < 10;}'
try 1 'main(){return 20 < 30;}'
try 0 'main(){return 20 <= 10;}'
try 1 'main(){return 20 <= 30;}'
try 1 'main(){return 20 <= 20;}'

try 1 'main(){return 20 > 10;}'
try 0 'main(){return 20 > 30;}'
try 1 'main(){return 20 >= 10;}'
try 0 'main(){return 20 >= 30;}'
try 1 'main(){return 20 >= 20;}'

try 3 'main(){return 7 % 4;}'
try 3 'main(){return (3+4) % 4;}'
try 0 'main(){return 10 % 10;}'

try 30 'main(){a=10;{b=20;}c=30; return c;}'
try 20 'main(){a=10;{a=20; return a;}c=30; return a;}'

try 20 'foo(){a=10;} main(){a=20; return a;}'
try 20 'foo(){a=10;} main(){foo(); return 20;}'
try 20 'foo(){a=10;} main(){a=20; foo();return a;}'
try 10 'foo(){a=10; return a;} main(){a=20; a=foo();return a;}'

try 40 'foo(a){print_int(a); return a*2;} main(){return foo(20);}'
try 40 'foo(a){return a*2;} main(){a=20; b=foo(a);return b;}'
try 85 'foo(a b c){return a+b+c;} main(){a=20; b=40; c=10; d=foo(a*2 b c/2);return d;}'

try 42 'main(){{} {;} ; return 42;}'

try 5 'main(){a=-5; b=+10; return a+b;}'

echo OK

