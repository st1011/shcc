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

echo OK

