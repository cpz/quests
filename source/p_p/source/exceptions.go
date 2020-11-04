package main

import (
	"errors"
)

func throw(err string) error {
	return errors.New(err)
}

func throwConvert(value string) error {
	return throw("cant convert \"" + value + "\" to desired type")
}
