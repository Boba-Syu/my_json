//
// Created by 19148 on 2023/4/11.
//
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include "my_json.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)\
    do {\
        test_count++;\
        if(equality) {\
            test_pass++;\
        } else {\
             fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret++;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_STRING(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%s")

#define TEST_ERROR(error, json)\
    do {\
        MyJSON myJson(JSON_NULL);\
        EXPECT_EQ_INT(error, myJson.parse(json));\
        EXPECT_EQ_INT(JSON_NULL, myJson.getType());\
    } while(0)

#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")

#define TEST_NUMBER(expect, json)\
    do {\
        MyJSON myJson(JSONType::JSON_TRUE);\
        EXPECT_EQ_INT(PARSE_OK, myJson.parse(json));\
        EXPECT_EQ_INT(JSON_NUMBER, myJson.getType());\
        EXPECT_EQ_DOUBLE(expect, myJson.getNumber());\
    } while(0)


static void test_parse_null() {
    MyJSON myJson(JSON_NULL);
    EXPECT_EQ_INT(PARSE_OK, myJson.parse("null"));
    EXPECT_EQ_INT(JSON_NULL, myJson.getType());
    std::cout << std::endl;
}

static void test_parse_true() {
    MyJSON myJson(JSON_NULL);
    EXPECT_EQ_INT(PARSE_OK, myJson.parse("true"));
    EXPECT_EQ_INT(JSON_TRUE, myJson.getType());
}

static void test_parse_false() {
    MyJSON myJson(JSON_NULL);
    EXPECT_EQ_INT(PARSE_OK, myJson.parse("false"));
    EXPECT_EQ_INT(JSON_FALSE, myJson.getType());
}

static void test_parse_expect_value() {
    TEST_ERROR(PARSE_EXPECT_VALUE, "");
    TEST_ERROR(PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(PARSE_INVALID_VALUE, "?");

    TEST_ERROR(PARSE_INVALID_VALUE, "[1,]");
//    TEST_ERROR(PARSE_INVALID_VALUE, "[\"a\", nul]");
    MyJSON myJson(JSON_NULL);
    EXPECT_EQ_INT(PARSE_INVALID_VALUE, myJson.parse("[\"a\", nul]"));
    EXPECT_EQ_INT(JSON_NULL, myJson.getType());
}

static void test_parse_root_not_singular() {
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "null x");
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "null false");
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_error() {
    test_parse_null();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_miss_comma_or_square_bracket();
}

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");

    TEST_ERROR(PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(PARSE_INVALID_VALUE, "nan");

    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0x123");

    TEST_ERROR(PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(PARSE_NUMBER_TOO_BIG, "-1e309");
}

#define TEST_STRING(expect, json)\
    do {\
        MyJSON myJson(JSONType::JSON_TRUE);\
        EXPECT_EQ_INT(PARSE_OK, myJson.parse(json));\
        EXPECT_EQ_INT(JSON_STRING, myJson.getType());\
        EXPECT_EQ_STRING(expect, myJson.getString());\
    } while(0)


static void test_parse_invalid_string_escape() {
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");

    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");

    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */

    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
}

static void test_access_boolean() {
    test_parse_true();
    test_parse_false();
}

static void test_access_number() {
    test_parse_number();
}

#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

static void test_parse_array() {
    MyJSON myJson;
    EXPECT_EQ_INT(PARSE_OK, myJson.parse("[ ]"));
    EXPECT_EQ_INT(JSON_ARRAY, myJson.getType());
    EXPECT_EQ_SIZE_T(0, myJson.getArray().size());
}

static void test_parse() {
    test_parse_error();
    test_parse_string();
    test_access_boolean();
    test_access_number();
    test_parse_array();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return 0;
}