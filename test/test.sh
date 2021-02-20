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

	../bin/shcc -dumptoken -dumpnode "$input" > testout.s
	gcc -g -o testout testout.s exfunc.o
	./testout
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "*** '$input'"
		echo "*** $expected expected, but got $actual (L$BASH_LINENO)"
		exit 1
	fi
}


try 0 'int main(){0;}'
try 42 'int main(){42;}'

try 42 'int main(){40+2;}'
try 21 'int main(){5+20-4;}'
try 0 'int main(){0+0-0;}'

try 21 'int main(){5 + 20 - 4;}'
try 42 'int main(){10+ 10 +22;}'

try 15 'int main(){3*5;}'
try 20 'int main(){60/3;}'
try 22 'int main(){4+6*3;}'
try 22 'int main(){6*3+4;}'
try 12 'int main(){(2+1)*4;}'
try 12 'int main(){4*(2+1);}'

try 47 'int main(){5+6*7;}'
try 15 'int main(){5*(9-6);}'
try 4 'int main(){(3+5)/2;}'

try 10 'int main(){int a; a=10;}'
try 22 'int main(){int a; a=10; int b; b=12 ;int c; c=a+b;}'
try 22 'int main(){int a; a=20; int a; a=10; int b; b=12 ;int c; c=a+b;}'
try 128 'int main(){int a; a=20;int a; a=10;int z; z=12;int y; y=30;int x; x=22;z*a+y-x;}'

try 128 'int main(){int a; a=20;int a; a=10;int z; z=12;int y; y=30;int x; x=22;return z*a+y-x;}'
try 12 'int main(){int a; a=20;int a; a=10;int z; z=12;int y; y=30;int x; x=22;z*a+y-x; return z;}'
try 20 'int main(){int a; a=20;return a;int a; a=10;int z; z=12;int y; y=30;int x; x=22;z*a+y-x;}'
try 20 'int main(){int a; a=20;return a;int a; a=10;return a;int z; z=12;int y; y=30;int x; x=22;z*a+y-x;}'

try 8 'int main(){int a;int b;int c;int d;a=b=c=d=2; return a+b+c+d;}'

try 10 'int main(){int abc; abc=10; return abc;}'
try 10 'int main(){int zyx; zyx=10; return zyx;}'
try 10 'int main(){int foobar; foobar=10; return foobar;}'
try 10 'int main(){int a123; a123=10; return a123;}'
try 10 'int main(){int a1; a1=2; int a2; a2=3; int a3; a3=4; int a4; a4=5; int a5; a5=6; int a6; a6=7; int a7; a7=8; int a8; a8=9; int a9; a9=10; int a10; a10=11; int a11; a11=12; int a12; a12=13; int a13; a13=14; int a14; a14=15; int a15; a15=16; int a16; a16=17; int a17; a17=18; int a18; a18=19; int a19; a19=20; int a20; a20=21; int a21; a21=22; int a22; a22=23; int a23; a23=24; int a24; a24=25; int a25; a25=26; int a26; a26=27; int a27; a27=28; int a28; a28=29; int a29; a29=30; int a30; a30=31; return a9;}'
try 0 'int main(){return 1==2;}'
try 1 'int main(){return 1==1;}'
try 0 'int main(){return 10!=10;}'
try 1 'int main(){return 10!=20;}'
try 1 'int main(){int foo; foo=10==10; return foo;}'
try 0 'int main(){int foo; foo=10!=10; return foo;}'

try 42 'int main(){exfunc1();return 42;}'
try 21 'int main(){exfunc2(42);return 21;}'
try 42 'int main(){exfunc2(1+2);return 42;}'
try 42 'int main(){exfunc3(42 21);return 42;}'
try 21 'int main(){exfunc3(21+21 15+6);return 21;}'
try 42 'int main(){int a; a=21;int b; b=42;exfunc3(a b);return 42;}'
try 21 'int main(){int a; a=21;int b; b=42;exfunc3(a+b b);return 21;}'

try 42 'int main(){return exfunc4();}'
try 42 'int main(){int a; a=exfunc4();return a;}'
try 42 'int main(){int a; a=exfunc4();int b; b=exfunc4();return a;}'
try 42 'int main(){int a; a=exfunc4();int b; b=exfunc4();return b;}'

try 42 'int main(){return exfunc5(40 2);}'
try 21 'int main(){int a; a=11;int b; b=10; return exfunc5(a b);}'
try 42 'int main(){return exfunc5(20+20 1+1);}'
try 42 'int main(){int a; a=exfunc5(20 1);int b; b=21; return a + b;}'
try 42 'int main(){int a; a=exfunc5(20 1);int b; b=exfunc5(20 1); int c; c=exfunc5(a b); return c;}'
try 42 'int main(){int a; a=exfunc5(20 1);int b; b=exfunc5(20 1); int c; return c=exfunc5(a b);}'

try 0 'int main(){return 20 < 10;}'
try 1 'int main(){return 20 < 30;}'
try 0 'int main(){return 20 <= 10;}'
try 1 'int main(){return 20 <= 30;}'
try 1 'int main(){return 20 <= 20;}'

try 1 'int main(){return 20 > 10;}'
try 0 'int main(){return 20 > 30;}'
try 1 'int main(){return 20 >= 10;}'
try 0 'int main(){return 20 >= 30;}'
try 1 'int main(){return 20 >= 20;}'

try 3 'int main(){return 7 % 4;}'
try 3 'int main(){return (3+4) % 4;}'
try 0 'int main(){return 10 % 10;}'

try 30 'int main(){int a; a=10;{int b; b=20;}int c; c=30; return c;}'
try 20 'int main(){int a; a=10;{int a; a=20; return a;}int c; c=30; return a;}'

try 20 'int foo(){;} int main(){int a; a=20; foo(); return a;}'
try 20 'int foo(){int a; a=10;} int main(){int a; a=20; return a;}'
try 20 'int foo(){int a; a=10;} int main(){foo(); return 20;}'
try 20 'int foo(){int a; a=10;} int main(){int a; a=20; foo();return a;}'
try 10 'int foo(){int a; a=10; return a;} int main(){int a; a=20; int a; a=foo();return a;}'

try 40 'int foo(int a){print_int(a); return a*2;} int main(){return foo(20);}'
try 40 'int foo(int a){return a*2;} int main(){int a; a=20; int b; b=foo(a);return b;}'
try 85 'int foo(int a int b int c){return a+b+c;} int main(){int a; a=20; int b; b=40; int c; c=10; int d; d=foo(a*2 b c/2);return d;}'

try 42 'int main(){{} {;} ; return 42;}'

try 5 'int main(){int a; a=-5; int b; b=+10; return a+b;}'

try 42 'int main(){if(1) return 42; return 0;}'
try 42 'int main(){if(0) return 0; return 42;}'
try 42 'int main(){if(0) return 0; else return 42; return 1;}'
try 42 'int main(){if(0) {return 0;} else {return 42;} return 1;}'
try 42 'int main(){if(0) {return 0;} else if (0) {return 0;} else { if (1) return 42;} return 1;}'

# 5!=120
try 120 'int fact(int n) { if (n==0) {return 1;} else { return fact(n-1) * n;}}int main() {int f; f=fact(5); return f;}'

try 42 'int main(){int i; for(i=0; i < 42; i=i + 1) {;} return i;}'
try 42 'int main(){int i; i=0; for(; i < 42; i=i + 1) {;} return i;}'
try 42 'int main(){int i; for(i=0; ; i=i + 1) {if (i == 42) {return i;}} return 0;}'
try 42 'int main(){int i; for(i=0; i < 42; ) {i+=1;} return i;}'

try 42 'int main(){int i; i=0; while (i<42) {i+=1;} return i;}'
try 42 'int main(){int i; i=0; while (i) {i+=1; return 0;} return 42;}'
try 42 'int main(){int i; i=1; while (i) {i+=1; return 42;} return 0;}'
try 42 'int main(){int i; i=0; while (1) {i+=1; if (i>=42) {return i;}} return 0;}'

try 42 'int main(){int a; a=10; int b; b=3; a+=b; return a + 29;}'
try 42 'int main(){int a; a=10; int b; b=3; a-=b; return a + 35;}'
try 42 'int main(){int a; a=10; int b; b=3; a*=b; return a + 12;}'
try 42 'int main(){int a; a=10; int b; b=3; a/=b; return a + 39;}'
try 42 'int main(){int a; a=10; int b; b=3; a%=b; return a + 41;}'

try 42 'int main(){int a; a=42;int b; b=&a; return *b;}'
# 変数は8Bアラインされている(というか8Bしかない)
# dは&bを指しているはず(配列がないので気持ち悪いがこんな感じのテストになる)
try 42 'int main(){int a; a=41; int b; b=42; int c; c=43; int d; d=&a-8; return *d;}'

echo OK

