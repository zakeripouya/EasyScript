package main

import "fmt"

func main() {
	total := 0
	for row := 1; row <= 10000; row++ {
		for column := 1; column <= 10000; column++ {
			if (row+column)%3 == 0 {
				total++
			}
		}
	}
	fmt.Println(total)
}
