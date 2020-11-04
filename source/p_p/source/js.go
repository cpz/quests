package main

import (
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

func (v *customObject) Init() error {
	var ok bool

	v.m, ok = v.decodedJSON.(map[string]interface{})

	if !ok {
		return throw("Can't convert interface in to map")
	}

	return nil
}

// Get - is private function for getting single key from json
func (v *customObject) Get(value string) (interface{}, error) {
	child, ok := v.Map()[value]
	if !ok {
		return 0, throw("cant get \"" + value + "\" value")
	}

	return child, nil
}

// GetMultiple - is private function for getting multiple keys from json
func (v *customObject) GetMultiple(value ...string) ([]interface{}, error) {
	var array []interface{}

	for _, key := range value {
		val, err := v.Get(key)
		if err == nil {
			array = append(array, val)
		} else {
			return array, throw("current value \"" + key + "\" is not found")
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
			return 0, throw("GetFloat: parsing \"" + v + "\": not a float")
		}

		return parsed, nil

	default:
		return 0, throw("GetFloat: unknown value")
	}
}

func (v *customObject) GetString(value string) (string, error) {
	child, err := v.Get(value)
	if err != nil {
		return "", err
	}

	parsedString, ok := child.(string)
	if !ok {
		return "", throwConvert(value)
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
		return false, throwConvert(value)
	}

	return parsedBool, nil
}

func (v *customObject) GetArray(value string) ([]interface{}, error) {
	child, err := v.Get(value)
	if err != nil {
		return nil, err
	}

	arr, ok := child.([]interface{})
	if !ok {
		return nil, throwConvert(value)
	}

	if len(arr) == 0 {
		return nil, throw("array with name \"" + value + "\" is empty")
	}

	return arr, nil
}
