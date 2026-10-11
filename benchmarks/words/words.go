package main

import (
	"fmt"
	"strconv"
)

func main() {
	vocabulary := make([]string, 0)
	for n := 0; n < 20000; n++ {
		vocabulary = append(vocabulary, "w"+strconv.Itoa(n))
	}
	counts := make(map[string]float64)
	x := int64(42)
	for i := 0; i < 2000000; i++ {
		x = x * 16807 % 2147483647
		word := vocabulary[x%20000]
		counts[word] = counts[word] + 1
	}
	fmt.Printf("%d %.0f %.0f %.0f\n", len(counts), counts["w0"], counts["w7"], counts["w19999"])
}
