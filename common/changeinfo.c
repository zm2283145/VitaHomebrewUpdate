#include "changeinfo.h"

static int bytes_equal(const unsigned char *input, size_t input_size,
                       size_t offset, const char *text, size_t text_size)
{
    size_t index;

    if (offset > input_size || text_size > input_size - offset)
        return 0;
    for (index = 0; index < text_size; ++index) {
        if (input[offset + index] != (unsigned char)text[index])
            return 0;
    }
    return 1;
}

static size_t find_bytes(const unsigned char *input, size_t input_size,
                         size_t offset, const char *text, size_t text_size)
{
    if (text_size == 0u || offset > input_size)
        return input_size;
    while (offset <= input_size - (input_size >= text_size ? text_size : 0u)) {
        if (bytes_equal(input, input_size, offset, text, text_size))
            return offset;
        ++offset;
        if (input_size < text_size)
            break;
    }
    return input_size;
}

static int is_space(unsigned char value)
{
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

static size_t utf8_sequence_size(const unsigned char *input, size_t remaining)
{
    unsigned char first;

    if (remaining == 0u)
        return 0u;
    first = input[0];
    if (first < 0x80u)
        return 1u;
    if (first >= 0xC2u && first <= 0xDFu && remaining >= 2u &&
        (input[1] & 0xC0u) == 0x80u)
        return 2u;
    if (first >= 0xE0u && first <= 0xEFu && remaining >= 3u &&
        (input[1] & 0xC0u) == 0x80u && (input[2] & 0xC0u) == 0x80u &&
        !(first == 0xE0u && input[1] < 0xA0u) &&
        !(first == 0xEDu && input[1] >= 0xA0u))
        return 3u;
    if (first >= 0xF0u && first <= 0xF4u && remaining >= 4u &&
        (input[1] & 0xC0u) == 0x80u && (input[2] & 0xC0u) == 0x80u &&
        (input[3] & 0xC0u) == 0x80u &&
        !(first == 0xF0u && input[1] < 0x90u) &&
        !(first == 0xF4u && input[1] >= 0x90u))
        return 4u;
    return 0u;
}

static size_t sanitized_size(const unsigned char *input, size_t input_size)
{
    size_t input_offset = 0u;
    size_t output_size = 0u;

    while (input_offset < input_size) {
        size_t sequence_size;
        unsigned char value = input[input_offset];

        if (value == '\r') {
            ++output_size;
            ++input_offset;
            if (input_offset < input_size && input[input_offset] == '\n')
                ++input_offset;
            continue;
        }
        if (value < 0x20u && value != '\n' && value != '\t') {
            ++output_size;
            ++input_offset;
            continue;
        }
        sequence_size = utf8_sequence_size(input + input_offset,
                                           input_size - input_offset);
        if (sequence_size == 0u) {
            ++output_size;
            ++input_offset;
        } else {
            output_size += sequence_size;
            input_offset += sequence_size;
        }
    }
    return output_size;
}

static size_t copy_sanitized(const unsigned char *input, size_t input_size,
                             char *output, size_t output_limit)
{
    size_t input_offset = 0u;
    size_t output_size = 0u;

    while (input_offset < input_size && output_size < output_limit) {
        size_t index;
        size_t sequence_size;
        unsigned char value = input[input_offset];

        if (value == '\r') {
            output[output_size++] = '\n';
            ++input_offset;
            if (input_offset < input_size && input[input_offset] == '\n')
                ++input_offset;
            continue;
        }
        if (value < 0x20u && value != '\n' && value != '\t') {
            output[output_size++] = '?';
            ++input_offset;
            continue;
        }
        sequence_size = utf8_sequence_size(input + input_offset,
                                           input_size - input_offset);
        if (sequence_size == 0u) {
            output[output_size++] = '?';
            ++input_offset;
            continue;
        }
        if (sequence_size > output_limit - output_size)
            break;
        for (index = 0u; index < sequence_size; ++index)
            output[output_size++] = (char)input[input_offset + index];
        input_offset += sequence_size;
    }
    return output_size;
}

int vhbu_changeinfo_extract(const unsigned char *xml, size_t xml_size,
                            char *output, size_t output_capacity)
{
    static const char root_open[] = "<changeinfo";
    static const char changes_open[] = "<changes><![CDATA[";
    static const char changes_close[] = "]]></changes>";
    static const char root_close[] = "</changeinfo>";
    size_t root;
    size_t root_end;
    size_t content;
    size_t content_end;
    size_t close;
    size_t required;
    size_t limit;
    size_t written;
    int truncated;

    if (output == NULL || output_capacity == 0u)
        return -1;
    output[0] = '\0';
    if (xml == NULL || xml_size == 0u)
        return -1;

    root = find_bytes(xml, xml_size, 0u, root_open,
                      sizeof(root_open) - 1u);
    if (root == xml_size)
        return -1;
    if (root + sizeof(root_open) - 1u >= xml_size ||
        (!is_space(xml[root + sizeof(root_open) - 1u]) &&
         xml[root + sizeof(root_open) - 1u] != '>'))
        return -1;
    root_end = find_bytes(xml, xml_size, root, ">", 1u);
    if (root_end == xml_size)
        return -1;
    content = find_bytes(xml, xml_size, root_end + 1u, changes_open,
                         sizeof(changes_open) - 1u);
    if (content == xml_size)
        return -1;
    content += sizeof(changes_open) - 1u;
    content_end = find_bytes(xml, xml_size, content, changes_close,
                             sizeof(changes_close) - 1u);
    if (content_end == xml_size)
        return -1;
    close = find_bytes(xml, xml_size,
                       content_end + sizeof(changes_close) - 1u,
                       root_close, sizeof(root_close) - 1u);
    if (close == xml_size)
        return -1;

    while (content < content_end && is_space(xml[content]))
        ++content;
    while (content_end > content && is_space(xml[content_end - 1u]))
        --content_end;
    if (content == content_end)
        return -1;

    required = sanitized_size(xml + content, content_end - content);
    truncated = required >= output_capacity;
    if (truncated && output_capacity < 4u)
        return -1;
    limit = truncated ? output_capacity - 4u : output_capacity - 1u;
    written = copy_sanitized(xml + content, content_end - content,
                             output, limit);
    if (truncated) {
        output[written++] = '.';
        output[written++] = '.';
        output[written++] = '.';
    }
    output[written] = '\0';
    return truncated ? VHBU_CHANGEINFO_TRUNCATED : VHBU_CHANGEINFO_OK;
}

int vhbu_changeinfo_extract_or_fallback(
    const unsigned char *xml, size_t xml_size, char *output,
    size_t output_capacity, const char *fallback)
{
    size_t index = 0u;
    int result = vhbu_changeinfo_extract(
        xml, xml_size, output, output_capacity);

    if (result >= 0)
        return result;
    if (output == NULL || output_capacity == 0u || fallback == NULL)
        return -1;
    while (index + 1u < output_capacity && fallback[index] != '\0') {
        output[index] = fallback[index];
        ++index;
    }
    output[index] = '\0';
    return -1;
}
