package main

import "fmt"

func main() {
	text := ""
	for i := 0; i < 10_000; i++ {
		text = text + "word "
	}
	fmt.Println(len(text))
}
