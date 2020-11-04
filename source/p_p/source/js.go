package main

import (
	"errors"
	"fmt"
	"strconv"
)

type customObject struct {
	decodedJSON interface{}
	m           map[string]interface{}
}

func (v *customObject) Map() map[string]interface{} {
	return v.m
}

func newObject(value interface{}) customObject {
	return customObject{
		decodedJSON: value,
	}
}

func (v *customObject) init() error {
	var ok bool

	v.m, ok = v.decodedJSON.(map[string]interface{})

	if !ok {
		return errors.New("Can't convert interface in to map")
	}

	return nil
}

// Get - is private function for getting single key from json
func (v *customObject) Get(value string) (interface{}, error) {
	child, ok := v.Map()[value]
	if !ok {
		return 0, errors.New("cant get \"" + value + "\" value")
	}

	return child, nil
}

// GetMultiple - is private function for getting multiple keys from json
func (v *customObject) GetMultiple(value ...string) ([]interface{}, error) {
	var array []interface{}

	for _, key := range value {
		fmt.Println(key)

		val, err := v.Get(key)
		if err == nil {
			array = append(array, val)
		} else {
			return array, errors.New("current value \"" + key + "\" is not found")
		}
	}

	return array, nil
}

func (v *customObject) GetFloat(value string) (float64, error) {
	rawValue, err := v.Get(value)
	if err != nil {
		return 0, err
	}

	switch v := rawValue.(type) {
	case int:
		return float64(v), nil

	case float64:
		return v, nil

	case string:
		parsed, err := strconv.ParseFloat(v, 64)
		if err != nil {
			return 0, errors.New("getValue: parsing \"" + v + "\": not a float")
		}

		return parsed, nil

	default:
		return 0, errors.New("getValue: unknown value")
	}
}

func (v *customObject) GetString(value string) (string, error) {
	child, err := v.Get(value)
	if err != nil {
		return "", err
	}

	parsedString, ok := child.(string)
	if !ok {
		return "", errors.New("cant convert \"" + value + "\" to string ")
	}

	return parsedString, nil
}

func (v *customObject) GetBool(value string) (bool, error) {
	child, err := v.Get(value)
	if err != nil {
		return false, err
	}

	parsedBool, ok := child.(bool)
	if !ok {
		return false, errors.New("cant convert \"" + value + "\" to boolean ")
	}

	return parsedBool, nil
}

func (v *customObject) GetArray(value string) ([]interface{}, error) {
	var result []interface{}

	child, err := v.Get(value)
	if err != nil {
		return nil, err
	}

	arr, ok := child.([]interface{})
	if !ok {
		return nil, errors.New("cant convert \"" + value + "\" to array")
	}

	if len(arr) == 0 {
		return nil, errors.New("array with name \"" + value + "\" is empty")
	}

	for _, v := range arr {
		result = append(result, v)
	}

	return result, nil
}
