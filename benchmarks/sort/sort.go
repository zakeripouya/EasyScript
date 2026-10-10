package main

import (
	"fmt"
	"sort"
)

func main() {
	numbers := make([]float64, 0)
	x := 42.0
	for i := 0; i < 1000000; i++ {
		x = float64(int64(x) * 16807 % 2147483647)
		numbers = append(numbers, x)
	}
	sort.Float64s(numbers)
	fmt.Printf("%.0f %.0f %.0f\n", numbers[0], numbers[499999], numbers[999999])
}
