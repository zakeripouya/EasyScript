package main

import (
	"fmt"
	"strings"
)

func main() {
	var text strings.Builder
	for i := 0; i < 1_000_000; i++ {
		text.WriteString("word ")
	}
	fmt.Println(len(text.String()))
}
