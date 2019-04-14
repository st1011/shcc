#!/bin/bash

cd `dirname $0`

try() {
	expected="$1"
	input="$2"

	../bin/shcc "$input" > out.s
	gcc -o out out.s
	./out
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$expected expected, but got $actual"
		exit 1
	fi
}

try 0 0
try 42 42

try 21 '5+20-4'
try 0 '0+0-0'

try 21 '5 + 20 - 4'
try 42 '10   + 10   + 22'

try 15 '3*5'
try 20 '60/3'
try 22 '4+6*3'
try 22 '6*3+4'
try 12 '(2+1)*4'
try 12 '4*(2+1)'

try 47 '5+6*7'
try 15 '5*(9-6)'
try 4 '(3+5)/2)'

echo OK

