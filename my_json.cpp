//
// Created by 19148 on 2023/4/12.
//
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
    if (ret == PARSE_OK) {
        value_.jVal = nullptr;
    }
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

JSONParseResult MyJSON::parseString(MyContext &context) {
    assert(*context.json == '\"');
    context.json++;
    const char *p = context.json;
    while (true) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                setString(context.chStack);
                context.json = p;
                type_ = JSON_STRING;
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

void MyJSON::setString(std::stack<char> &chStack) {
    while (!chStack.empty()) {
        value_.sVal = chStack.top() + value_.sVal;
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
            context.json++;
            type_ = JSON_ARRAY;
            return PARSE_OK;
        } else
            return PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
    }
}

JSONParseResult MyJSON::parseObject(MyJSON::MyContext &context) {
    assert(*context.json == '{');
    JSONParseResult ret = PARSE_OK;

    return ret;
}
