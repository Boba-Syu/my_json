//
// Created by 19148 on 2023/4/11.
//

#ifndef MY_JSON_MY_JSON_H
#define MY_JSON_MY_JSON_H

#include <string>
#include <cassert>
#include <iostream>
#include <vector>
#include <stack>

enum JSONType {
    JSON_NULL, JSON_FALSE, JSON_TRUE, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT
};

enum JSONParseResult {
    PARSE_OK,
    PARSE_EXPECT_VALUE,
    PARSE_INVALID_VALUE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_NUMBER_TOO_BIG,
    PARSE_MISS_QUOTATION_MARK,
    PARSE_INVALID_STRING_ESCAPE,
    PARSE_INVALID_STRING_CHAR,
    PARSE_INVALID_UNICODE_SURROGATE,
    PARSE_INVALID_UNICODE_HEX,
    PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    PARSE_MISS_KEY,
    PARSE_MISS_COLON,
    PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

class MyJSON;

struct JsonMember {
    std::string key;
    std::shared_ptr<MyJSON> value;
};

class MyJSON {
public:
    explicit MyJSON(JSONType type = JSON_NULL) : type_(type) {}

    JSONParseResult parse(const char *);

    JSONType getType() { return type_; }

    double getNumber();

    std::string getString();

    std::vector<MyJSON> getArray();

    std::vector<JsonMember> getJsonObject();

private:

    struct JSONValue {
        double nVal;
        std::string sVal;
        std::vector<JsonMember> jVal;
        std::vector<MyJSON> arrVal;

        JSONValue() : nVal(0), sVal(""), jVal({}), arrVal({}) {}
    };

    struct MyContext {
        const char *json;
        std::stack<char> chStack;

        MyContext() : json(nullptr), chStack(std::stack<char>()) {}
    };

    JSONType type_;
    JSONValue value_;

    static void parseWhitespace(MyContext &);

    static void encodeUTF8(MyContext &context, unsigned int u);

    JSONParseResult parseNull(MyContext &);

    JSONParseResult parseValue(MyContext &);

    JSONParseResult parseTrue(MyContext &);

    JSONParseResult parseFalse(MyContext &);

    JSONParseResult parseValue(MyContext &, const char *, JSONType);

    JSONParseResult parseNumber(MyContext &);

    JSONParseResult parseString(MyContext &);

    JSONParseResult parseStringRaw(MyContext &, std::string &);

    JSONParseResult parseObject(MyContext &);

    void setString(std::stack<char> &chStack, std::string &);

    JSONParseResult parseArray(MyContext &context);

};

#endif //MY_JSON_MY_JSON_H
