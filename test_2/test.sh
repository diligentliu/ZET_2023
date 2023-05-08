#!/bin/bash

g++ test.cpp -o test
g++ determine_2.cpp -o determine_2

min_a=-10
max_a=10
min_b=-10
max_b=10
d=0.1
best_a=$min_a
best_b=$min_b
./test $best_a $best_b
best_return=$(./determine_2)

# 清空文件内容
> results.csv

a=$min_a
while (( $(echo "$a <= $max_a" |bc -l) )); do
    b=$min_b
    while (( $(echo "$b <= $max_b" |bc -l) )); do
        ./test $a $b
        return_val=$(./determine_2)
        echo "$a,$b,$return_val"
        echo "$a,$b,$return_val" >> results.csv
        if (( $(echo "$return_val < $best_return" |bc -l) )); then
            best_return=$return_val
            best_a=$a
            best_b=$b
        fi
        b=$(echo "$b + $d" |bc)
    done
    a=$(echo "$a + $d" |bc)
done

echo "Best a: $best_a"
echo "Best b: $best_b"
echo "Best return value: $best_return"