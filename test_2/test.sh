#!/bin/bash

# compile C++ executable
g++ test.cpp -o test
min=0.7
max=0.9
d=0.01
# set initial min_result to a very large number
./test $min
min_result=$(./determine_2)

# loop through range of a values and run test_2
for a in $(seq -f "%.3f" $min $d $max); do
	./test $a
    result=$(./determine_2)
    echo "$result"
    # compare current result to min_result
    if (( $(echo "$result <= $min_result" | bc -l) )); then
        min_result=$result
        min_a=$a
    fi
done

echo "Minimum result is $min_result for a=$min_a"
