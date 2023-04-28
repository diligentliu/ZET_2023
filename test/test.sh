#!/bin/bash

# Compile the C++ program
g++ test.cpp -o test
g++ ../determine_2/determine_2.cpp -o determine_2

# Set initial values for minimum a and minimum result
min_a=-10
./test $min_a
min_result=$(./determine_2)

# Loop through all values of a
for a in $(seq -f "%.1f" -9.9 0.1 10); do
    # Run test program with current a value
    ./test $a

    # Run determine_2 program and get the result
    result=$(./determine_2)

    # Check if current result is less than minimum result
    if (( $(echo "$result < $min_result" | bc -l) )); then
        min_result=$result
        min_a=$a
    fi

    # Print the result for the current a value
    printf "Result for a=%.1f: %.6f\n" $a $result
done

# Print the minimum result and the value of a that produced it
printf "Minimum result is %.6f for a=%.1f\n" $min_result $min_a
