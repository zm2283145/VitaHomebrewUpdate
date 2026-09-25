#include "update_config.h"

#include <psp2/kernel/clib.h>

static size_t bounded_length(const char *text, size_t capacity)
{
    size_t length = 0;
    if (text == NULL)
        return 0;
    while (length < capacity && text[length] != '\0')
        ++length;
    return length;
}

static int copy_text(char *destination, size_t capacity,
                     const char *source, size_t length)
{
    if (destination == NULL || capacity == 0 || source == NULL ||
        length >= capacity)
        return -1;
    sceClibMemcpy(destination, source, length);
    destination[length] = '\0';
    return 0;
}

static int text_equals(const char *left, size_t left_size,
                       const char *right)
{
    size_t right_size = bounded_length(right, left_size + 1u);
    return right_size == left_size &&
           sceClibStrncmp(left, right, left_size) == 0;
}

static int valid_title_id(const char *title_id)
{
    size_t index;
    if (bounded_length(title_id, 12u) != 9u)
        return 0;
    for (index = 0; index < 9u; ++index) {
        char value = title_id[index];
        if (!((value >= 'A' && value <= 'Z') ||
              (value >= '0' && value <= '9')))
            return 0;
    }
    return 1;
}

void vhbu_update_config_clear(VhbuUpdateConfig *config)
{
    if (config != NULL)
        sceClibMemset(config, 0, sizeof(*config));
}

int vhbu_update_config_parse_ini(VhbuUpdateConfig *config,
                                 const char *text, size_t size)
{
    size_t offset = 0;
    int have_title = 0;
    int have_url = 0;

    if (config == NULL || text == NULL)
        return -1;
    while (offset < size) {
        size_t start = offset;
        size_t end;
        size_t equals;
        size_t value;

        while (offset < size && text[offset] != '\n' && text[offset] != '\r')
            ++offset;
        end = offset;
        while (offset < size && (text[offset] == '\n' || text[offset] == '\r'))
            ++offset;
        while (start < end && (text[start] == ' ' || text[start] == '\t'))
            ++start;
        while (end > start && (text[end - 1u] == ' ' || text[end - 1u] == '\t'))
            --end;
        if (start == end || text[start] == '#' || text[start] == ';')
            continue;
        equals = start;
        while (equals < end && text[equals] != '=')
            ++equals;
        if (equals == end)
            continue;
        value = equals + 1u;
        while (value < end && (text[value] == ' ' || text[value] == '\t'))
            ++value;
        while (equals > start &&
               (text[equals - 1u] == ' ' || text[equals - 1u] == '\t'))
            --equals;
        if (text_equals(text + start, equals - start, "title_id")) {
            if (copy_text(config->title_id, sizeof(config->title_id),
                          text + value, end - value) < 0)
                return -2;
            have_title = 1;
        } else if (text_equals(text + start, equals - start, "name")) {
            if (copy_text(config->name, sizeof(config->name),
                          text + value, end - value) < 0)
                return -3;
        } else if (text_equals(text + start, equals - start, "update_url")) {
            if (copy_text(config->update_url, sizeof(config->update_url),
                          text + value, end - value) < 0)
                return -4;
            have_url = 1;
        }
    }
    if (!have_title || !valid_title_id(config->title_id))
        return -5;
    if (!have_url ||
        (sceClibStrncmp(config->update_url, "https://", 8u) != 0 &&
         sceClibStrncmp(config->update_url, "http://", 7u) != 0))
        return -6;
    if (config->name[0] == '\0')
        copy_text(config->name, sizeof(config->name), config->title_id, 9u);
    return 0;
}

static const char *find_text(const char *text, size_t size,
                             const char *needle)
{
    size_t needle_size = bounded_length(needle, 128u);
    size_t index;
    if (needle_size == 0 || needle_size > size)
        return NULL;
    for (index = 0; index + needle_size <= size; ++index) {
        if (sceClibStrncmp(text + index, needle, needle_size) == 0)
            return text + index;
    }
    return NULL;
}

static int copy_attribute(const char *element, const char *limit,
                          const char *name, char *output, size_t capacity)
{
    char needle[64];
    const char *value;
    const char *end;
    int length;

    length = sceClibSnprintf(needle, sizeof(needle), "%s=\"", name);
    if (length <= 0 || length >= (int)sizeof(needle))
        return -1;
    value = find_text(element, (size_t)(limit - element), needle);
    if (value == NULL)
        return -2;
    value += (size_t)length;
    end = value;
    while (end < limit && *end != '\"')
        ++end;
    if (end == limit)
        return -3;
    return copy_text(output, capacity, value, (size_t)(end - value));
}

static int parse_u64(const char *text, uint64_t *value)
{
    uint64_t result = 0;
    size_t index = 0;
    if (text == NULL || value == NULL || text[0] == '\0')
        return -1;
    while (text[index] != '\0') {
        unsigned int digit;
        if (text[index] < '0' || text[index] > '9')
            return -2;
        digit = (unsigned int)(text[index] - '0');
        if (result > (UINT64_MAX - digit) / 10u)
            return -3;
        result = result * 10u + digit;
        ++index;
    }
    *value = result;
    return 0;
}

static int sha1_is_valid(const char *text)
{
    size_t index;
    if (bounded_length(text, 42u) != 40u)
        return 0;
    for (index = 0; index < 40u; ++index) {
        char value = text[index];
        if (!((value >= '0' && value <= '9') ||
              (value >= 'a' && value <= 'f') ||
              (value >= 'A' && value <= 'F')))
            return 0;
    }
    return 1;
}

int vhbu_update_config_parse_xml(VhbuUpdateConfig *config,
                                 const char *text, size_t size)
{
    const char *package;
    const char *package_end;
    const char *changeinfo;
    const char *changeinfo_end;
    char size_text[32];

    if (config == NULL || text == NULL)
        return -1;
    package = find_text(text, size, "<package");
    if (package == NULL)
        return -2;
    package_end = package;
    while ((size_t)(package_end - text) < size && *package_end != '>')
        ++package_end;
    if ((size_t)(package_end - text) >= size)
        return -3;
    if (copy_attribute(package, package_end, "version",
                       config->available_version,
                       sizeof(config->available_version)) < 0 ||
        copy_attribute(package, package_end, "size", size_text,
                       sizeof(size_text)) < 0 ||
        copy_attribute(package, package_end, "sha1sum",
                       config->package_sha1,
                       sizeof(config->package_sha1)) < 0 ||
        copy_attribute(package, package_end, "url", config->package_url,
                       sizeof(config->package_url)) < 0 ||
        copy_attribute(package, package_end, "content_id",
                       config->content_id, sizeof(config->content_id)) < 0)
        return -4;
    if (parse_u64(size_text, &config->package_size) < 0 ||
        config->package_size == 0 || !sha1_is_valid(config->package_sha1))
        return -5;
    changeinfo = find_text(package_end, size - (size_t)(package_end - text),
                           "<changeinfo");
    if (changeinfo == NULL)
        return -6;
    changeinfo_end = changeinfo;
    while ((size_t)(changeinfo_end - text) < size && *changeinfo_end != '>')
        ++changeinfo_end;
    if ((size_t)(changeinfo_end - text) >= size ||
        copy_attribute(changeinfo, changeinfo_end, "url",
                       config->changeinfo_url,
                       sizeof(config->changeinfo_url)) < 0)
        return -7;
    return 0;
}

static const char *url_filename(const char *url)
{
    const char *result = url;
    const char *cursor = url;
    while (cursor != NULL && *cursor != '\0') {
        if (*cursor == '/')
            result = cursor + 1;
        ++cursor;
    }
    return result;
}

int vhbu_update_config_build_paths(VhbuUpdateConfig *config,
                                   const char *data_directory)
{
    const char *filename;
    int result;
    if (config == NULL || data_directory == NULL ||
        !valid_title_id(config->title_id) ||
        config->available_version[0] == '\0')
        return -1;
    filename = url_filename(config->package_url);
    if (filename == NULL || filename[0] == '\0' ||
        bounded_length(filename, sizeof(config->package_name)) >=
            sizeof(config->package_name))
        return -2;
    result = copy_text(config->package_name, sizeof(config->package_name),
                       filename, bounded_length(filename,
                       sizeof(config->package_name)));
    if (result < 0)
        return result;
    result = sceClibSnprintf(config->icon_path, sizeof(config->icon_path),
        "ux0:app/%s/sce_sys/icon0.png", config->title_id);
    if (result <= 0 || result >= (int)sizeof(config->icon_path))
        return -3;
    result = sceClibSnprintf(config->pending_path,
        sizeof(config->pending_path), "%s/%s", data_directory,
        config->package_name);
    if (result <= 0 || result >= (int)sizeof(config->pending_path))
        return -4;
    result = sceClibSnprintf(config->stage_path, sizeof(config->stage_path),
        "%s/stage-%s-%s", data_directory, config->title_id,
        config->available_version);
    if (result <= 0 || result >= (int)sizeof(config->stage_path))
        return -5;
    return 0;
}

int vhbu_compare_versions(const char *left, const char *right)
{
    size_t left_offset = 0;
    size_t right_offset = 0;
    while ((left != NULL && left[left_offset] != '\0') ||
           (right != NULL && right[right_offset] != '\0')) {
        uint64_t left_value = 0;
        uint64_t right_value = 0;
        while (left != NULL && left[left_offset] == '.')
            ++left_offset;
        while (right != NULL && right[right_offset] == '.')
            ++right_offset;
        while (left != NULL && left[left_offset] >= '0' &&
               left[left_offset] <= '9') {
            left_value = left_value * 10u +
                         (unsigned int)(left[left_offset++] - '0');
        }
        while (right != NULL && right[right_offset] >= '0' &&
               right[right_offset] <= '9') {
            right_value = right_value * 10u +
                          (unsigned int)(right[right_offset++] - '0');
        }
        if (left_value < right_value)
            return -1;
        if (left_value > right_value)
            return 1;
        if ((left == NULL || left[left_offset] == '\0') &&
            (right == NULL || right[right_offset] == '\0'))
            return 0;
        if ((left != NULL && left[left_offset] != '.' &&
             left[left_offset] != '\0') ||
            (right != NULL && right[right_offset] != '.' &&
             right[right_offset] != '\0'))
            return 0;
    }
    return 0;
}
