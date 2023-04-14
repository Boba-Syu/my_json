//
// Created by 19148 on 2023/4/12.
//
#include <algorithm>
#include "my_json.h"

double MyJSON::getNumber() {
    assert(type_ == JSON_NUMBER);
    return value_.nVal;
}

std::vector<MyJSON> MyJSON::getArray() {
    assert(type_ == JSON_ARRAY);
    return value_.arrVal;
}

JSONParseResult MyJSON::parse(const char *json) {
    MyContext context;
    context.json = json;
    type_ = JSON_NULL;
    parseWhitespace(context);
    JSONParseResult ret = parseValue(context);
    if (ret == PARSE_OK) {
        parseWhitespace(context);
        if (*context.json != '\0') {
            type_ = JSON_NULL;
            ret = PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}


void MyJSON::parseWhitespace(MyContext &context) {
    const char *p = context.json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    context.json = p;
}

JSONParseResult MyJSON::parseValue(MyContext &context) {
    switch (*context.json) {
        case 'n':
            return parseNull(context);
        case 't':
            return parseTrue(context);
        case 'f':
            return parseFalse(context);
        case '\"':
            return parseString(context);
        case '[':
            return parseArray(context);
        case '{':
            return parseObject(context);
        case '\0':
            return PARSE_EXPECT_VALUE;
        default:
            return parseNumber(context);
    }
}

JSONParseResult MyJSON::parseNull(MyContext &context) {
    const char *value = "null";
    JSONParseResult ret = parseValue(context, value, JSON_NULL);

    return ret;
}

JSONParseResult MyJSON::parseTrue(MyContext &context) {
    const char *value = "true";
    JSONParseResult ret = parseValue(context, value, JSON_TRUE);
    if (ret == PARSE_OK) {
        type_ = JSON_TRUE;
    }
    return ret;
}

JSONParseResult MyJSON::parseFalse(MyContext &context) {
    const char *value = "false";
    JSONParseResult ret = parseValue(context, value, JSON_TRUE);
    if (ret == PARSE_OK) {
        type_ = JSON_FALSE;
    }
    return ret;
}

JSONParseResult MyJSON::parseValue(MyContext &context, const char *value, JSONType type) {
    assert(*context.json == *value);
    context.json++;
    size_t size = strlen(value) - 1;
    for (int i = 0; i < size; i++) {
        if (context.json[i] != value[i + 1]) {
            return PARSE_INVALID_VALUE;
        }
    }
    context.json += size;
    type_ = type;
    return PARSE_OK;
}

inline bool isDigit(const char ch) {
    return ch >= '0' && ch <= '9';
}

inline bool isDigit1to9(const char ch) {
    return ch >= '1' && ch <= '9';
}

JSONParseResult MyJSON::parseNumber(MyContext &context) {
    const char *p = context.json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!isDigit1to9(*p)) return PARSE_INVALID_VALUE;
        do { p++; } while (isDigit(*p));
    }
    if (*p == '.') {
        p++;
        if (!isDigit(*p)) return PARSE_INVALID_VALUE;
        do { p++; } while (isDigit(*p));
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!isDigit(*p)) return PARSE_INVALID_VALUE;
        do { p++; } while (isDigit(*p));
    }
    errno = 0;
    value_.nVal = strtod(context.json, nullptr);
    if (errno == ERANGE && (value_.nVal == HUGE_VAL || value_.nVal == -HUGE_VAL)) {
        return PARSE_NUMBER_TOO_BIG;
    }

    context.json = p;
    type_ = JSONType::JSON_NUMBER;
    return PARSE_OK;
}

inline bool isHexDigitUpperChar(const char ch) {
    return 'A' < ch && ch < 'F';
}

inline bool isHexDigitLowerChar(const char ch) {
    return 'a' < ch && ch < 'f';
}

inline bool isHexDigit(const char ch) {
    return isDigit(ch) || isHexDigitUpperChar(ch) || isHexDigitLowerChar(ch);
}

void getHexDigit(const char p, unsigned &u) {
    assert(isHexDigit(p));
    if (isDigit(p)) {
        u = u * 16 + (p - '0');
    } else if (isHexDigitLowerChar(p)) {
        u = u * 16 + (p - 'a');
    } else {
        u = u * 16 + (p - 'A');
    }
}

bool parse_hex4(const char *&p, unsigned &u) {
    char *end;
    u = (unsigned) strtol(p, &end, 16);
    if (end == p + 4) {
        p += 4;
        return true;
    }
    return false;
}

void MyJSON::encodeUTF8(MyContext &context, unsigned u) {
    if (u <= 0x7f)
        context.chStack.push(u & 0xff);
    else if (u <= 0x7ff) {
        context.chStack.push(0xc0 | ((u >> 6) & 0xff));
        context.chStack.push(0x80 | (u & 0x3f));
    } else if (u <= 0xffff) {
        context.chStack.push(0xe0 | ((u >> 12) & 0xff));
        context.chStack.push(0x80 | ((u >> 6)) & 0x3f);
        context.chStack.push(0x80 | (u & 0x3f));
    } else {
        assert(u <= 0x10ffff);
        context.chStack.push(0xf0 | (u >> 18) & 0xff);
        context.chStack.push(0x80 | (u >> 12) & 0x3f);
        context.chStack.push(0x80 | ((u >> 6)) & 0x3f);
        context.chStack.push(0x80 | (u) & 0x3f);
    }
}

JSONParseResult MyJSON::parseStringRaw(MyJSON::MyContext &context, std::string &value) {
    assert(*context.json == '\"');
    context.json++;
    const char *p = context.json;
    while (true) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                setString(context.chStack, value);
                context.json = p;
                return PARSE_OK;
            case '\0':
                return PARSE_MISS_QUOTATION_MARK;
            case '\\': {
                char ch = *p++;
                switch (ch) {
                    case 'n':
                        context.chStack.push('\n');
                        break;
                    case '\\':
                        context.chStack.push('\\');
                        break;
                    case '\"':
                        context.chStack.push('\"');
                        break;
                    case '/':
                        context.chStack.push('/');
                        break;
                    case 'b':
                        context.chStack.push('\b');
                        break;
                    case 'f':
                        context.chStack.push('\f');
                        break;
                    case 'r':
                        context.chStack.push('\r');
                        break;
                    case 't':
                        context.chStack.push('\t');
                        break;
                    case 'u':
                        unsigned u;
                        if (!parse_hex4(p, u)) return PARSE_INVALID_UNICODE_HEX;
                        if (u >= 0xd800 && u < 0xdbff) {
                            if (*p++ != '\\')return PARSE_INVALID_UNICODE_SURROGATE;
                            if (*p++ != 'u')return PARSE_INVALID_UNICODE_SURROGATE;
                            unsigned u2;
                            if (!parse_hex4(p, u2)) return PARSE_INVALID_UNICODE_HEX;
                            if (u2 < 0xd800 || u2 > 0xdfff) return PARSE_INVALID_UNICODE_SURROGATE;
                            u = (((u - 0xd800) << 10) | (u2 - 0xdc00)) + 0x10000;
                        }
                        encodeUTF8(context, u);
                        break;
                    default:
                        return PARSE_INVALID_STRING_ESCAPE;
                }
                break;
            }
            default:
                if (ch < 0x20) return PARSE_INVALID_STRING_CHAR;
                context.chStack.push(ch);
        }
    }
}

JSONParseResult MyJSON::parseString(MyContext &context) {
    auto ret = parseStringRaw(context, value_.sVal);
    if (ret == PARSE_OK) {
        type_ = JSON_STRING;
    }
    return ret;
}

void MyJSON::setString(std::stack<char> &chStack, std::string &value) {
    value = "";
    while (!chStack.empty()) {
        value = chStack.top() + value;
        chStack.pop();
    }
}

std::string MyJSON::getString() {
    assert(type_ == JSON_STRING);
    return value_.sVal;
}

JSONParseResult MyJSON::parseArray(MyContext &context) {
    assert(*context.json == '[');
    context.json++;
    JSONParseResult ret = PARSE_OK;
    parseWhitespace(context);
    if (*context.json == ']') {
        context.json++;
        type_ = JSON_ARRAY;
        return PARSE_OK;
    }
    while (true) {
        MyJSON subJson;
        ret = subJson.parseValue(context);
        parseWhitespace(context);
        if (ret != PARSE_OK) {
            return ret;
        }
        if (*context.json == ',') {
            value_.arrVal.push_back(subJson);
            context.json++;
        } else if (*context.json == ']') {
            value_.arrVal.push_back(subJson);
            context.json++;
            type_ = JSON_ARRAY;
            return PARSE_OK;
        } else
            return PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
    }
}

JSONParseResult MyJSON::parseObject(MyJSON::MyContext &context) {
    assert(*context.json == '{');
    context.json++;
    JSONParseResult ret = PARSE_OK;
    parseWhitespace(context);
    if (*context.json == '}') {
        context.json++;
        type_ = JSON_OBJECT;
        return ret;
    }
    std::string key = "";
    while (true) {
        // 解析key
        MyJSON value;
        if (*context.json != '"') return PARSE_MISS_KEY;
        ret = parseStringRaw(context, key);
        if (ret != PARSE_OK) break;

        // 冒号
        parseWhitespace(context);
        if (*context.json != ':') return PARSE_MISS_COLON;
        context.json++;
        parseWhitespace(context);

        // 解析value
        if (key.empty()) return PARSE_MISS_KEY;
        ret = value.parseValue(context);
        if (ret != PARSE_OK) break;
        value_.jVal[key] = value;
        parseWhitespace(context);

        // 是否又下一个键值对
        if (*context.json == ',') {
            context.json++;
            parseWhitespace(context);
        } else if (*context.json == '}') {
            // 该object解析完了
            context.json++;
            type_ = JSON_OBJECT;
            return PARSE_OK;
        } else {
            return PARSE_MISS_COMMA_OR_CURLY_BRACKET;
        }
    }

    return ret;
}

JSONStringifyResult MyJSON::jsonStringify(char *&json) {
    std::string sjson = "";
    auto ret = jsonStringify(sjson);
    json = sjson.data();
    return ret;
}

JSONStringifyResult MyJSON::jsonStringify(std::string &json) {
    std::string sjson = "";
    JSONStringifyResult ret = STRINGIFY_OK;
    switch (type_) {
        case JSON_TRUE:
            ret = trueStringify(sjson);
            break;
        case JSON_FALSE:
            ret = falseStringify(sjson);
            break;
        case JSON_NULL:
            ret = nullStringify(sjson);
            break;
        case JSON_NUMBER:
            ret = numberStringify(sjson);
            break;
        case JSON_STRING:
            ret = stringStringify(sjson);
            break;
        case JSON_ARRAY:
            ret = arrayStringify(sjson);
            break;
        case JSON_OBJECT:
            ret = objectStringify(sjson);
            break;
    }
    json += sjson;
    return ret;
}

JSONStringifyResult MyJSON::trueStringify(std::string &sjson) {
    sjson += "true";
    return STRINGIFY_OK;
}

JSONStringifyResult MyJSON::falseStringify(std::string &sjson) {
    sjson += "false";
    return STRINGIFY_OK;
}

JSONStringifyResult MyJSON::nullStringify(std::string &sjson) {
    sjson += "null";
    return STRINGIFY_OK;
}

JSONStringifyResult MyJSON::numberStringify(std::string &sjson) {
    assert(type_ == JSON_NUMBER);
    char buffer[32];
    sprintf(buffer, "%.17g", value_.nVal);
    sjson += buffer;
    return STRINGIFY_OK;
}

JSONStringifyResult MyJSON::stringStringifyRaw(std::string &sjson, const std::string &value) {
    sjson += '"';
    for (char ch: value) {
        switch (ch) {
            case '\"':
                sjson += "\\\"";
                break;
            case '\n':
                sjson += "\\n";
                break;
            case '\\':
                sjson += "\\\\";
                break;
            case '\b':
                sjson += "\\b";
                break;
            case '\f':
                sjson += "\\f";
                break;
            case '\r':
                sjson += "\\r";
                break;
            case '\t':
                sjson += "\\t";
                break;
            default:
                if (ch < 0x20) {
                    char buffer[7];
                    sprintf(buffer, "\\u%04X", ch);
                    sjson += buffer;
                } else
                    sjson += ch;
        }
    }
    sjson += '"';
    return STRINGIFY_OK;
}

JSONStringifyResult MyJSON::stringStringify(std::string &sjson) {
    assert(type_ == JSON_STRING);
    stringStringifyRaw(sjson, value_.sVal);
    return STRINGIFY_OK;
}

JSONStringifyResult MyJSON::arrayStringify(std::string &sjson) {
    assert(type_ == JSON_ARRAY);
    auto ret = STRINGIFY_OK;
    sjson += '[';
    for (auto iter = value_.arrVal.begin(); iter != value_.arrVal.end(); iter++) {
        ret = iter->jsonStringify(sjson);
        if ((iter + 1) != value_.arrVal.end()) {
            sjson += ',';
        }
    }
    sjson += ']';
    return ret;
}

JSONStringifyResult MyJSON::objectStringify(std::string &sjson) {
    assert(type_ == JSON_OBJECT);
    auto ret = STRINGIFY_OK;
    sjson += '{';
    for (auto [key, value]: value_.jVal) {
        ret = stringStringifyRaw(sjson, key);
        sjson += ':';
        ret = value.jsonStringify(sjson);
    }
    sjson += '}';
    return ret;
}

std::vector<std::string> MyJSON::getKeys() {
    assert(type_ = JSON_OBJECT);
    std::vector<std::string> ret;
    std::for_each(value_.jVal.begin(), value_.jVal.end(),
                  [&ret](std::pair<std::string, MyJSON> member) { ret.push_back(member.first); });
    return ret;
}

MyJSON MyJSON::getValueFromKey(std::string key) {
    if (type_ == JSON_OBJECT) {
        for (auto [k, v]: value_.jVal) {
            if (k == key) {
                return v;
            }
        }
    }
    throw std::exception("json don't has that key");
}

void MyJSON::setValueToKey(std::string key, MyJSON myJson) {
    assert(type_ == JSON_OBJECT);
    value_.jVal[key] = myJson;
}

bool arrEquals(const std::vector<MyJSON> &arr1, const std::vector<MyJSON> &arr2) {
    int ret = arr1.size() == arr2.size();
    if (ret) {
        for (int i = 0; i < arr1.size(); i++) {
            ret = ret && arr1[i] == arr2[i];
        }
    }
    return ret;
}


bool objEquals(std::map<std::string, MyJSON> map1, std::map<std::string, MyJSON> map2) {
    bool ret = map1.size() == map2.size();
    if (ret) {
        for (auto [key, value]: map1) {
            ret = ret && value == map2[key];
        }
    }
    return ret;
}


bool MyJSON::operator==(const MyJSON &json) const {
    bool ret = type_ == json.type_;
    if (ret) {
        switch (type_) {
            case JSON_TRUE:
            case JSON_FALSE:
            case JSON_NULL:
                return ret;
            case JSON_NUMBER:
                return value_.nVal == json.value_.nVal;
            case JSON_STRING:
                return value_.sVal == json.value_.sVal;
            case JSON_ARRAY:
                return arrEquals(value_.arrVal, json.value_.arrVal);
            case JSON_OBJECT:
                return objEquals(value_.jVal, value_.jVal);
        }
    }
    return ret;
}
