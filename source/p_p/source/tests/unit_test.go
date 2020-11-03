package main

import (
	"encoding/json"
	"errors"
	"io/ioutil"
	"os"
	"regexp"
	"strconv"
	"testing"

	"github.com/stretchr/testify/assert"
)

func decodeJSON(data []byte) (interface{}, error) {
	var value interface{}
	err := json.Unmarshal(data, &value)
	return value, err
}

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

func TestFile(t *testing.T) {
	path, err := os.Getwd()
	assert.NoError(t, err)

	path += "\\p_p.json"

	assert.FileExists(t, path)

	file, err := os.Open(path)
	defer file.Close()

	assert.NoError(t, err)

	_, err = ioutil.ReadFile(path)
	assert.NoError(t, err)

	path += ".kek"

	assert.NoFileExists(t, path)
}

func TestJson(t *testing.T) {
	json := `{"object":"value"}`

	_, err := decodeJSON([]byte(json))
	assert.NoError(t, err)

	_, err = decodeJSON([]byte("{1}"))
	assert.Error(t, err)
}

func TestRawFloatValue(t *testing.T) {
	_, err := getflValue("1")
	assert.NoError(t, err)

	_, err = getflValue(1)
	assert.NoError(t, err)

	_, err = getflValue(1.1337)
	assert.NoError(t, err)

	_, err = getflValue(false)
	assert.Error(t, err)

	_, err = getflValue("1.a")
	assert.Error(t, err)

	_, err = getflValue("1.33aaa")
	assert.Error(t, err)

	_, err = getflValue("1a")
	assert.Error(t, err)

	_, err = getflValue("a")
	assert.Error(t, err)
}

func TestTelephone(t *testing.T) {
	var invalid map[int]string = map[int]string{
		1: "89146661337c",
		2: "7kek6661337",
		3: "1337",
		4: "79I466613E7",
		5: "",
	}

	var valid map[int]string = map[int]string{
		1: "79146661337",
		2: "+79146661337",
		3: "8(914)6661337",
		4: "8 (914) 6661337",
		5: "+7 (914) 666-13-37",
	}

	re := regexp.MustCompile(`^(\+7|7|8)?[\s\-]?\(?[489][0-9]{2}\)?[\s\-]?[0-9]{3}[\s\-]?[0-9]{2}[\s\-]?[0-9]{2}$`)

	if assert.NotNil(t, valid, "valid is map of valid telephone numbers, valid numbers should match to regexp") &&
		assert.NotNil(t, invalid, "invalid is map of invalid telephone numbers, ivalid numbers should not match to regexp") {
		for _, value := range invalid {
			assert.NotRegexp(t, re, value)
		}

		for _, value := range valid {
			assert.Regexp(t, re, value)
		}
	}
}
