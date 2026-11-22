#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace qlqpc::webui
{
    enum class overlay_asset_kind
    {
        none,
        index_html,
        overlay_js
    };

    struct rgb_color
    {
        int r{255};
        int g{255};
        int b{255};

        bool operator==(const rgb_color& other) const = default;
    };

    overlay_asset_kind classify_asset_request(
        std::string_view host,
        std::string_view url_path);

    std::optional<std::string> inject_overlay_into_index_html(std::string_view html);

    std::optional<std::string> extract_web_pak_text_resource_from_file(
        const std::filesystem::path& pak_path,
        std::string_view resource_name);

    std::optional<std::string> load_text_file(const std::filesystem::path& path);

    std::string format_ql_hex_color(int r, int g, int b);

    std::optional<rgb_color> parse_ql_hex_color(std::string_view text);
}
