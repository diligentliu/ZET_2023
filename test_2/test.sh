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

a=$min_a
while (( $(echo "$a <= $max_a" |bc -l) )); do
    b=$min_b
    while (( $(echo "$b <= $max_b" |bc -l) )); do
        ./test $a $b
        return_val=$(./determine_2)
        echo "$return_val"
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

##!/bin/bash
#
## compile C++ executable
#g++ test.cpp -o test
#min=0.7
#max=0.9
#d=0.01
## set initial min_result to a very large number
#./test $min
#min_result=$(./determine_2)
#
## loop through range of a values and run test_2
#for a in $(seq -f "%.3f" $min $d $max); do
#	./test $a
#    result=$(./determine_2)
#    echo "$result"
#    # compare current result to min_result
#    if (( $(echo "$result <= $min_result" | bc -l) )); then
#        min_result=$result
#        min_a=$a
#    fi
#done
#
#echo "Minimum result is $min_result for a=$min_a"
