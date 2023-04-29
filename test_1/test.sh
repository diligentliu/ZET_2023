#!/bin/bash

g++ test.cpp -o test
g++ determine_1.cpp -o determine_1

best_a=-2.00
best_b=-1.00
./test $a $b
best_return=$(./determine_1)

a=-2.00
while (( $(echo "$a <= 0.00" |bc -l) )); do
  b=-1.00
  while (( $(echo "$b <= 1.00" |bc -l) )); do
  	./test $a $b
    return_val=$(./determine_1)
    if (( $(echo "$return_val < $best_return" |bc -l) )); then
      best_return=$return_val
      best_a=$a
      best_b=$b
    fi
    b=$(echo "$b + 0.1" |bc)
  done
  a=$(echo "$a + 0.1" |bc)
done

echo "Best a: $best_a"
echo "Best b: $best_b"
echo "Best return value: $best_return"