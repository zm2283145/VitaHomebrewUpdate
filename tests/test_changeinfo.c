#include <stdio.h>
#include <string.h>

#include "changeinfo.h"

static int failures;

static void expect_result(const char *name, const unsigned char *xml,
                          size_t xml_size, size_t capacity,
                          int expected_result, const char *expected_output)
{
    char output[128];
    int result;

    memset(output, 0x5a, sizeof(output));
    result = vhbu_changeinfo_extract(xml, xml_size, output, capacity);
    if (result != expected_result ||
        strcmp(output, expected_output) != 0) {
        fprintf(stderr, "%s: result=%d output=\"%s\"\n",
                name, result, output);
        ++failures;
    }
}

int main(void)
{
    static const unsigned char valid[] =
        "<?xml version=\"1.0\"?><changeinfo><changes><![CDATA[\r\n"
        "- Fixes updates\r\n- Adds caf\xc3\xa9 support\r\n"
        "]]></changes></changeinfo>";
    static const unsigned char malformed[] =
        "<changeinfo><changes>- Not CDATA</changes></changeinfo>";
    static const unsigned char missing[] =
        "<changeinfo></changeinfo>";
    static const unsigned char invalid_utf8[] =
        "<changeinfo><changes><![CDATA[Good \xc0\xaf bad\001text"
        "]]></changes></changeinfo>";
    char tiny[3] = "xx";
    char fallback[64];

    expect_result("valid", valid, sizeof(valid) - 1u, 128u,
                  VHBU_CHANGEINFO_OK,
                  "- Fixes updates\n- Adds caf\xc3\xa9 support");
    expect_result("bounded", valid, sizeof(valid) - 1u, 20u,
                  VHBU_CHANGEINFO_TRUNCATED, "- Fixes updates\n...");
    expect_result("malformed", malformed, sizeof(malformed) - 1u, 128u,
                  -1, "");
    expect_result("missing", missing, sizeof(missing) - 1u, 128u,
                  -1, "");
    expect_result("sanitized", invalid_utf8,
                  sizeof(invalid_utf8) - 1u, 128u,
                  VHBU_CHANGEINFO_OK, "Good ?? bad?text");
    if (vhbu_changeinfo_extract_or_fallback(
            malformed, sizeof(malformed) - 1u, fallback, sizeof(fallback),
            "Release notes are unavailable for this update.") != -1 ||
        strcmp(fallback,
               "Release notes are unavailable for this update.") != 0) {
        fprintf(stderr, "malformed XML did not use the explicit fallback\n");
        ++failures;
    }
    if (vhbu_changeinfo_extract(valid, sizeof(valid) - 1u,
                                tiny, sizeof(tiny)) != -1 ||
        tiny[0] != '\0') {
        fprintf(stderr, "tiny output did not fail safely\n");
        ++failures;
    }
    if (failures != 0)
        return 1;
    puts("changeinfo tests passed");
    return 0;
}
