// Functions given to built-ins: map, filter and reduce over a million numbers.
package main

import "fmt"

func mapInts(xs []int, f func(int) int) []int {
	out := make([]int, len(xs))
	for i, x := range xs {
		out[i] = f(x)
	}
	return out
}

func filterInts(xs []int, f func(int) bool) []int {
	out := []int{}
	for _, x := range xs {
		if f(x) {
			out = append(out, x)
		}
	}
	return out
}

func reduceInts(xs []int, f func(int, int) int, start int) int {
	acc := start
	for _, x := range xs {
		acc = f(acc, x)
	}
	return acc
}

func main() {
	numbers := []int{}
	x := 42
	for i := 0; i < 1000000; i++ {
		x = x * 16807 % 2147483647
		numbers = append(numbers, x%1000)
	}
	total := 0
	for round := 0; round < 10; round++ {
		doubled := mapInts(numbers, func(n int) int { return n * 2 })
		kept := filterInts(doubled, func(n int) bool { return n%3 == 0 })
		total += reduceInts(kept, func(a, b int) int { return a + b }, 0)
		numbers = mapInts(numbers, func(n int) int { return n + 1 })
	}
	fmt.Println(total)
}
