#ifndef RC_INI_PARSER_INI_HPP
#define RC_INI_PARSER_INI_HPP

#include <unordered_map>

#include <File/File.hpp>
#include <ParserBase/Tokenizer.hpp>
#include <ParserBase/TokenParser.hpp>
#include <ParserBase/Token.hpp>
#include <IniParser/Common.hpp>
#include <IniParser/Section.hpp>

namespace RC::Ini
{
    class Value;

    class Parser
    {
    public:
        enum class CanThrow
        {
            Yes,
            No,
        };

    private:
        std::unordered_map<File::StringType, Section> m_sections;
        bool m_parsing_is_complete{false};

    public:
        Parser() = default;

    private:
        RC_INI_PARSER_API auto parse_internal(File::StringType& input) -> void;
        RC_INI_PARSER_API auto create_available_tokens_for_tokenizer() -> ParserBase::TokenContainer;
        RC_INI_PARSER_API auto get_value(const File::StringType& section, const File::StringType& key, CanThrow = CanThrow::Yes) const -> std::optional<std::reference_wrapper<const Value>>;

    public:
        RC_INI_PARSER_API auto parse(File::StringType& input) -> void;
        RC_INI_PARSER_API auto parse(const File::Handle&) -> void;
        RC_INI_PARSER_API auto get_list(const File::StringType& section) -> List;
        RC_INI_PARSER_API auto get_ordered_list(const File::StringType& section) -> List;
        RC_INI_PARSER_API auto get_string(const File::StringType& section, const File::StringType& key, const File::StringType& default_value) const noexcept -> const File::StringType&;
        RC_INI_PARSER_API auto get_string(const File::StringType& section, const File::StringType& key) const  -> const File::StringType&;
        // Should there be more integer getters ? They'd confirm size and convert explicitly (or throw?), no implicit conversions.
        RC_INI_PARSER_API auto get_int64(const File::StringType& section, const File::StringType& key, int64_t default_value) const noexcept -> int64_t;
        RC_INI_PARSER_API auto get_int64(const File::StringType& section, const File::StringType& key) const -> int64_t;
        RC_INI_PARSER_API auto get_float(const File::StringType& section, const File::StringType& key, float default_value) const noexcept -> float;
        RC_INI_PARSER_API auto get_float(const File::StringType& section, const File::StringType& key) const -> float;
        RC_INI_PARSER_API auto get_bool(const File::StringType& section, const File::StringType& key, bool default_value) const noexcept -> bool;
        RC_INI_PARSER_API auto get_bool(const File::StringType& section, const File::StringType& key) const -> bool;
    };
}

#endif //RC_INI_PARSER_INI_HPP
