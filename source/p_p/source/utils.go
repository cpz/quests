package main

import (
	"encoding/json"
	"errors"
	"regexp"
	"strconv"
)

// DecodeJSON - I guess you should understand what there happens, right?
func decodeJSON(data []byte) (interface{}, error) {
	var value interface{}
	err := json.Unmarshal(data, &value)
	return value, err
}

// GetflValue - Since we can get json object as "1.0" or 1.0 - we should handle it properly
//
// Value can be as string or float64 (notice: 1 is considered as float64 too instead of int)
//
// if we cant convert\handle value prorerly then:
//
// 1. return value is 0
//
// 2. error will be string which should tell you whats wrong
func getflValue(value interface{}) (float64, error) {
	switch v := value.(type) {
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

type unknown0 struct {
	Bar     string   `json:"bar"`
	Baz     string   `json:"baz"`
	Foo     float64  `json:"foo"`
	Foz     []string `json:"foz"`
	unknown map[string]interface{}
}

// getData - converts decoded json data in to struct with proper handling members in it
//
// Returns filled struct unknown0
//
// or
//
// error
//
// note: unknown is map of json values which we dont have in struct but we received them.
func getData(data interface{}) (unknown0, error) {
	unk0 := unknown0{
		Bar:     "",
		Baz:     "",
		Foo:     0,
		Foz:     []string{},
		unknown: map[string]interface{}{},
	}

	m, ok := data.(map[string]interface{})
	if !ok {
		return unk0, errors.New("Can't convert interface in to map")
	}

	for key, value := range m {
		if key == "foo" {
			floatParsedValue, err := getflValue(value)
			if err != nil {
				return unk0, errors.New("Key 'Foo' is not a number")
			}

			unk0.Foo = floatParsedValue
		} else if key == "bar" {
			barValue, ok := value.(string)
			if !ok {
				return unk0, errors.New("Key 'Bar' is not valid")
			}

			unk0.Bar = barValue
		} else if key == "baz" {
			bazValue, ok := value.(string)
			if !ok {
				return unk0, errors.New("Key 'Baz' is not valid")
			}

			re := regexp.MustCompile(`^(\+7|7|8)?[\s\-]?\(?[489][0-9]{2}\)?[\s\-]?[0-9]{3}[\s\-]?[0-9]{2}[\s\-]?[0-9]{2}$`)

			if !re.MatchString(bazValue) {
				return unk0, errors.New("Key 'Baz' is not valid russian telephone number")
			}

			unk0.Baz = bazValue
		} else if key == "foz" {
			for _, v := range value.([]interface{}) {
				rawString, ok := v.(string)

				if ok {
					unk0.Foz = append(unk0.Foz, rawString)
				}
			}
		} else {
			unk0.unknown[key] = value
		}
	}

	return unk0, nil
}
