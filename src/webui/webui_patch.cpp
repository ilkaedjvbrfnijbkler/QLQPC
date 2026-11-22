#include "webui/webui_patch.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <span>
#include <system_error>
#include <vector>

namespace qlqpc::webui
{
    namespace
    {
        struct datapack_entry_span
        {
            uint32_t begin{0};
            uint32_t end{0};
        };

        constexpr uint32_t datapack_v4_version = 4u;
        constexpr uint32_t datapack_header_bytes = 9u;
        constexpr uint32_t datapack_entry_bytes = 6u;
        constexpr uint32_t max_pak_entries = 1'000'000u;
        constexpr uintmax_t max_read_file_bytes = 64u * 1024u * 1024u;
        constexpr uint64_t max_datapack_file_bytes = 0xFFFFFFFFull;
        constexpr std::string_view js_script_tag = "<script src=\"qlqpc_settings.js\"></script>";

        bool equals_ignore_ascii_case(std::string_view a, std::string_view b)
        {
            if (a.size() != b.size())
            {
                return false;
            }
            for (size_t i = 0; i < a.size(); ++i)
            {
                const auto ca = static_cast<unsigned char>(a[i]);
                const auto cb = static_cast<unsigned char>(b[i]);
                if (std::tolower(ca) != std::tolower(cb))
                {
                    return false;
                }
            }
            return true;
        }

        size_t find_ignore_ascii_case(
            std::string_view haystack,
            std::string_view needle,
            size_t start_pos = 0)
        {
            if (needle.empty())
            {
                return std::min(start_pos, haystack.size());
            }
            if (start_pos >= haystack.size() || needle.size() > haystack.size() - start_pos)
            {
                return std::string_view::npos;
            }
            const size_t limit = haystack.size() - needle.size();
            for (size_t i = start_pos; i <= limit; ++i)
            {
                if (equals_ignore_ascii_case(haystack.substr(i, needle.size()), needle))
                {
                    return i;
                }
            }
            return std::string_view::npos;
        }

        template <typename T>
        bool read_le(std::span<const uint8_t> bytes, size_t offset, T& out)
        {
            if (offset > bytes.size() || sizeof(T) > bytes.size() - offset)
            {
                return false;
            }
            std::memcpy(&out, bytes.data() + offset, sizeof(T));
            return true;
        }

        int hex_digit_value(char ch)
        {
            if (ch >= '0' && ch <= '9')
            {
                return ch - '0';
            }
            const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if (lower >= 'a' && lower <= 'f')
            {
                return 10 + (lower - 'a');
            }
            return -1;
        }

        int parse_hex_byte(std::string_view two_chars)
        {
            if (two_chars.size() < 2)
            {
                return -1;
            }
            const int hi = hex_digit_value(two_chars[0]);
            const int lo = hex_digit_value(two_chars[1]);
            if (hi < 0 || lo < 0)
            {
                return -1;
            }
            return (hi << 4) | lo;
        }

        bool validate_datapack_header(
            std::span<const uint8_t> header_bytes,
            uint64_t archive_size,
            uint32_t& out_count,
            size_t& out_table_bytes)
        {
            if (header_bytes.size() < datapack_header_bytes || archive_size < datapack_header_bytes)
            {
                return false;
            }
            uint32_t version = 0;
            uint32_t count = 0;
            if (!read_le(header_bytes, 0, version) || !read_le(header_bytes, 4, count))
            {
                return false;
            }
            if (version != datapack_v4_version || count == 0 || count > max_pak_entries)
            {
                return false;
            }
            const size_t table_entries = static_cast<size_t>(count) + 1u;
            const size_t table_bytes = table_entries * datapack_entry_bytes;
            if (table_bytes > archive_size - datapack_header_bytes)
            {
                return false;
            }
            out_count = count;
            out_table_bytes = table_bytes;
            return true;
        }

        std::optional<datapack_entry_span> find_datapack_entry_span(
            std::span<const uint8_t> table_bytes,
            uint32_t count,
            uint64_t archive_size,
            uint16_t resource_id)
        {
            const size_t min_data_offset =
                static_cast<size_t>(datapack_header_bytes) +
                (static_cast<size_t>(count) + 1u) * datapack_entry_bytes;

            for (uint32_t i = 0; i < count; ++i)
            {
                const size_t cur_off = static_cast<size_t>(i) * datapack_entry_bytes;
                const size_t next_off = cur_off + datapack_entry_bytes;

                uint16_t rid = 0;
                uint32_t data_begin = 0;
                uint32_t data_end = 0;
                if (!read_le(table_bytes, cur_off, rid) ||
                    !read_le(table_bytes, cur_off + 2u, data_begin) ||
                    !read_le(table_bytes, next_off + 2u, data_end))
                {
                    return std::nullopt;
                }
                if (rid != resource_id)
                {
                    continue;
                }
                if (data_begin < min_data_offset ||
                    data_end < data_begin ||
                    static_cast<uint64_t>(data_end) > archive_size)
                {
                    return std::nullopt;
                }
                return datapack_entry_span{data_begin, data_end};
            }

            return std::nullopt;
        }

        std::optional<std::vector<uint8_t>> read_pak_entry_from_stream(
            std::ifstream& ifs,
            datapack_entry_span span)
        {
            static_assert(sizeof(std::streamoff) >= 8, "DataPak stream seeking requires 64-bit streamoff");
            const uint64_t byte_len = static_cast<uint64_t>(span.end) - span.begin;
            if (byte_len > max_read_file_bytes)
            {
                return std::nullopt;
            }
            ifs.clear();
            ifs.seekg(static_cast<std::streamoff>(span.begin), std::ios::beg);
            if (!ifs)
            {
                return std::nullopt;
            }
            std::vector<uint8_t> buf(static_cast<size_t>(byte_len));
            if (!buf.empty())
            {
                ifs.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
                if (!ifs)
                {
                    return std::nullopt;
                }
            }
            return buf;
        }

        std::string_view strip_url_query_and_fragment(std::string_view path)
        {
            const size_t q = path.find_first_of("?#");
            return (q == std::string_view::npos) ? path : path.substr(0, q);
        }

        std::optional<uint16_t> lookup_pickle_resource_id(
            std::span<const uint8_t> pickle_bytes,
            std::string_view resource_name)
        {
            uint32_t payload_size = 0;
            uint32_t val_type = 0;
            uint32_t version = 0;
            uint32_t map_count = 0;
            if (!read_le(pickle_bytes, 0, payload_size) ||
                !read_le(pickle_bytes, 4, val_type) ||
                !read_le(pickle_bytes, 8, version) ||
                !read_le(pickle_bytes, 12, map_count))
            {
                return std::nullopt;
            }

            if (payload_size < 12u || payload_size > pickle_bytes.size() - 4u ||
                val_type != 7u || map_count > max_pak_entries)
            {
                return std::nullopt;
            }

            const size_t end_pos = 4u + static_cast<size_t>(payload_size);
            size_t pos = 16u;
            for (uint32_t i = 0; i < map_count; ++i)
            {
                uint32_t char16_len = 0;
                if (!read_le(pickle_bytes, pos, char16_len))
                {
                    return std::nullopt;
                }
                pos += 4u;
                if (char16_len > (std::numeric_limits<size_t>::max() - 3u) / 2u)
                {
                    return std::nullopt;
                }
                const size_t raw_str_bytes = static_cast<size_t>(char16_len) * 2u;
                // Chromium Pickle aligns each field payload to a 4-byte boundary.
                const size_t padded_str_bytes = (raw_str_bytes + 3u) & ~size_t{3};
                if (padded_str_bytes > end_pos - pos)
                {
                    return std::nullopt;
                }

                bool name_matches = (static_cast<size_t>(char16_len) == resource_name.size());
                if (name_matches)
                {
                    for (size_t c = 0; c < resource_name.size(); ++c)
                    {
                        uint16_t code_unit = 0;
                        if (!read_le(pickle_bytes, pos + c * 2u, code_unit))
                        {
                            return std::nullopt;
                        }
                        if (code_unit > 0x7Fu ||
                            std::tolower(static_cast<unsigned char>(code_unit)) !=
                                std::tolower(static_cast<unsigned char>(resource_name[c])))
                        {
                            name_matches = false;
                            break;
                        }
                    }
                }
                pos += padded_str_bytes;

                uint32_t entry_type = 0;
                uint32_t entry_rid = 0;
                if (8u > end_pos - pos ||
                    !read_le(pickle_bytes, pos, entry_type) ||
                    !read_le(pickle_bytes, pos + 4u, entry_rid))
                {
                    return std::nullopt;
                }
                pos += 8u;

                if (name_matches)
                {
                    if (entry_rid > 0xFFFFu)
                    {
                        return std::nullopt;
                    }
                    return static_cast<uint16_t>(entry_rid);
                }
            }

            return std::nullopt;
        }

        std::optional<std::vector<uint8_t>> load_binary_file(const std::filesystem::path& path)
        {
            if (path.empty())
            {
                return std::nullopt;
            }
            std::error_code ec;
            const uintmax_t size = std::filesystem::file_size(path, ec);
            if (ec || size > max_read_file_bytes)
            {
                return std::nullopt;
            }
            std::ifstream ifs(path, std::ios::binary);
            if (!ifs.is_open())
            {
                return std::nullopt;
            }
            std::vector<uint8_t> data(static_cast<size_t>(size));
            if (!data.empty())
            {
                ifs.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
                if (!ifs)
                {
                    return std::nullopt;
                }
            }
            return data;
        }

    }

    overlay_asset_kind classify_asset_request(
        std::string_view host,
        std::string_view url_path)
    {
        if (!equals_ignore_ascii_case(host, "ql"))
        {
            return overlay_asset_kind::none;
        }
        std::string_view clean = strip_url_query_and_fragment(url_path);
        while (!clean.empty() && clean.front() == '/')
        {
            clean.remove_prefix(1);
        }
        if (equals_ignore_ascii_case(clean, "index.html"))
        {
            return overlay_asset_kind::index_html;
        }
        if (equals_ignore_ascii_case(clean, "qlqpc_settings.js"))
        {
            return overlay_asset_kind::overlay_js;
        }
        return overlay_asset_kind::none;
    }

    std::optional<std::string> inject_overlay_into_index_html(std::string_view html)
    {
        std::string out(html);

        if (find_ignore_ascii_case(out, "qlqpc_settings.js") == std::string_view::npos)
        {
            const std::string insertion = std::string("  ") + std::string(js_script_tag) + "\n";
            const size_t bundle_pos = find_ignore_ascii_case(out, "bundle.js");
            if (bundle_pos == std::string_view::npos)
            {
                return std::nullopt;
            }
            const size_t close_script = find_ignore_ascii_case(out, "</script>", bundle_pos);
            if (close_script == std::string_view::npos)
            {
                return std::nullopt;
            }
            size_t js_pos = close_script + std::string_view("</script>").size();
            if (js_pos < out.size() && out[js_pos] == '\r')
            {
                ++js_pos;
            }
            if (js_pos < out.size() && out[js_pos] == '\n')
            {
                ++js_pos;
            }
            else
            {
                out.insert(js_pos, "\n");
                ++js_pos;
            }
            out.insert(js_pos, insertion);
        }

        return out;
    }

    std::optional<std::string> extract_web_pak_text_resource_from_file(
        const std::filesystem::path& pak_path,
        std::string_view resource_name)
    {
        if (pak_path.empty())
        {
            return std::nullopt;
        }
        std::error_code ec;
        const uintmax_t file_bytes = std::filesystem::file_size(pak_path, ec);
        if (ec || file_bytes < datapack_header_bytes || file_bytes > max_datapack_file_bytes)
        {
            return std::nullopt;
        }

        std::ifstream ifs(pak_path, std::ios::binary);
        if (!ifs.is_open())
        {
            return std::nullopt;
        }

        uint8_t header[datapack_header_bytes]{};
        ifs.read(reinterpret_cast<char*>(header), sizeof(header));
        if (!ifs)
        {
            return std::nullopt;
        }

        const uint64_t archive_size = static_cast<uint64_t>(file_bytes);
        uint32_t count = 0;
        size_t table_bytes = 0;
        if (!validate_datapack_header(header, archive_size, count, table_bytes) ||
            table_bytes > max_read_file_bytes)
        {
            return std::nullopt;
        }

        std::vector<uint8_t> toc(table_bytes);
        ifs.read(reinterpret_cast<char*>(toc.data()), static_cast<std::streamsize>(toc.size()));
        if (!ifs)
        {
            return std::nullopt;
        }

        const auto index_span = find_datapack_entry_span(toc, count, archive_size, 0u);
        if (!index_span)
        {
            return std::nullopt;
        }
        const auto index_pickle = read_pak_entry_from_stream(ifs, *index_span);
        if (!index_pickle)
        {
            return std::nullopt;
        }

        const auto rid = lookup_pickle_resource_id(*index_pickle, resource_name);
        if (!rid)
        {
            return std::nullopt;
        }
        const auto payload_span = find_datapack_entry_span(toc, count, archive_size, *rid);
        if (!payload_span)
        {
            return std::nullopt;
        }
        const auto payload = read_pak_entry_from_stream(ifs, *payload_span);
        if (!payload)
        {
            return std::nullopt;
        }

        return std::string(reinterpret_cast<const char*>(payload->data()), payload->size());
    }

    std::optional<std::string> load_text_file(const std::filesystem::path& path)
    {
        const auto bytes = load_binary_file(path);
        if (!bytes)
        {
            return std::nullopt;
        }
        return std::string(reinterpret_cast<const char*>(bytes->data()), bytes->size());
    }

    std::string format_ql_hex_color(int r, int g, int b)
    {
        constexpr char hex_digits[] = "0123456789abcdef";
        const int cr = std::clamp(r, 0, 255);
        const int cg = std::clamp(g, 0, 255);
        const int cb = std::clamp(b, 0, 255);

        std::string out("0x000000ff");
        out[2] = hex_digits[(cr >> 4) & 0xF];
        out[3] = hex_digits[cr & 0xF];
        out[4] = hex_digits[(cg >> 4) & 0xF];
        out[5] = hex_digits[cg & 0xF];
        out[6] = hex_digits[(cb >> 4) & 0xF];
        out[7] = hex_digits[cb & 0xF];
        return out;
    }

    std::optional<rgb_color> parse_ql_hex_color(std::string_view text)
    {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
        {
            text.remove_prefix(1);
        }
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
        {
            text.remove_suffix(1);
        }

        if (text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
        {
            text.remove_prefix(2);
        }
        else if (!text.empty() && text[0] == '#')
        {
            text.remove_prefix(1);
        }

        if (text.size() != 6u && text.size() != 8u)
        {
            return std::nullopt;
        }
        for (const char ch : text)
        {
            if (hex_digit_value(ch) < 0)
            {
                return std::nullopt;
            }
        }

        const int r = parse_hex_byte(text.substr(0, 2));
        const int g = parse_hex_byte(text.substr(2, 2));
        const int b = parse_hex_byte(text.substr(4, 2));
        if (r < 0 || g < 0 || b < 0)
        {
            return std::nullopt;
        }
        return rgb_color{r, g, b};
    }
}
