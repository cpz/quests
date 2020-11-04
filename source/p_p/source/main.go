package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"os"
)

func main() {
	fmt.Println("p_p - readin' json file and workin' with it in Go lang")

	path, err := os.Getwd()
	if err != nil {
		log.Println(err)
	}

	path += "\\p_p.json"

	file, err := os.Open(path)
	if err != nil {
		log.Fatal(err)
	}
	defer file.Close()

	data, err := ioutil.ReadFile(path)
	if err != nil {
		log.Fatal(err)
	}

	ret, err := decodeJSON([]byte(data))
	if err != nil {
		log.Fatal(err)
	}

	res, err := getData(ret)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("JSON:\n\tFoo \t: \t%v\n\tBar \t: \t%v\n\tBaz \t: \t%v\n\tQoo \t: \t%v\n", res.Foo, res.Bar, res.Baz, res.Qoo)

	if len(res.Foz) != 0 {
		fmt.Printf("\tFoz \t: \t%+v\n", res.Foz)
	}

	fmt.Println("g_G - press enter to quit")
	fmt.Scanln()
}
