package main

import (
	"encoding/json"
	"fmt"
	"regexp"
)

// DecodeJSON - I guess you should understand what there happens, right?
func decodeJSON(data []byte) (interface{}, error) {
	var value interface{}
	err := json.Unmarshal(data, &value)
	return value, err
}

type unknown0 struct {
	Bar string        `json:"bar"`
	Baz string        `json:"baz"`
	Foo float64       `json:"foo"`
	Foz []interface{} `json:"foz"`
	Qoo bool          `json:"Qoo"`
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
		Bar: "",
		Baz: "",
		Foo: 0,
		Foz: []interface{}{},
	}

	js := newObject(data)

	err := js.Init()
	if err != nil {
		return unk0, throw("cant initialize custom json")
	}

	unk0.Foo, err = js.GetFloat("foo")
	if err != nil {
		fmt.Println(err)
		return unk0, throw("GetFloat foo has failed")
	}

	unk0.Bar, err = js.GetString("bar")
	if err != nil {
		return unk0, throw("GetString bar has failed")
	}

	unk0.Baz, err = js.GetString("baz")
	if err == nil {
		re := regexp.MustCompile(`^(\+7|7|8)?[\s\-]?\(?[489][0-9]{2}\)?[\s\-]?[0-9]{3}[\s\-]?[0-9]{2}[\s\-]?[0-9]{2}$`)

		if !re.MatchString(unk0.Baz) {
			return unk0, throw("Key 'baz' is not valid russian telephone number")
		}
	} else {
		return unk0, throw("GetString baz has failed")
	}

	unk0.Qoo, err = js.GetBool("Qoo")
	if err != nil {
		return unk0, throw("GetBool qoo has failed")
	}

	unk0.Foz, err = js.GetArray("foz")
	if err != nil {
		return unk0, throw("GetArray foz has failed")
	}

	return unk0, nil
}
