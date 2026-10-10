package main

import "fmt"

func main() {
	total := 0
	for i := 1; i <= 100_000_000; i++ {
		total += i
	}
	fmt.Println(total / 1_000_000)
}
