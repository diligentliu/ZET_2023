#!/bin/bash

g++ ./test.cpp -o test

best_a=-10.3
best_b=-10.3
best_c=-10.3
best_return=9999999

a=2.3
while (( $(echo "$a <= 1.0" | bc -l) )); do
    b=2.3
    while (( $(echo "$b <= 1.0" | bc -l) )); do
        c=2.3
        while (( $(echo "$c <= 1.0" | bc -l) )); do
            return_val=$(./test $a $b $c)
            if (( $(echo "$return_val < $best_return" | bc -l) )); then
                best_return=$return_val
                best_a=$a
                best_b=$b
                best_c=$c
            fi
            c=$(echo "$c + 0.01" | bc)
        done
        b=$(echo "$b + 0.01" | bc)
    done
    a=$(echo "$a + 0.01" | bc)
done

echo "Best a: $best_a"
echo "Best b: $best_b"
echo "Best c: $best_c"
echo "Best return value: $best_return"
