#pragma once

#include <memory>
#include <string>
#include <vector>

#include <File/Macros.hpp>
#include <ParserBase/Common.hpp>

namespace RC::ParserBase
{
    class TokenRule
    {
      private:
        SystemStringType m_debug_name;

      public:
        explicit TokenRule(SystemStringViewType rule_name) : m_debug_name(rule_name)
        {
        }
        virtual ~TokenRule() = default;

      public:
        RC_PB_API virtual auto exec(const class Token& token, const SystemCharType* start_of_token, size_t current_cursor_location, class Tokenizer&) -> int = 0;

        [[nodiscard]] RC_PB_API virtual auto to_string() const -> SystemStringType
        {
            return SYSSTR("BaseTokenRule (invalid)");
        }
    };

    class Token
    {
      private:
        friend class Tokenizer;

      public:
        enum class HasData
        {
            Yes,
            No,
        };

      private:
        SystemStringType m_debug_name;
        SystemStringType m_identifier;
        std::vector<std::shared_ptr<TokenRule>> m_rules;
        int m_type; // To be cast to an enum before use. This is to avoid using a template which forces everything to be in the header file.
        mutable size_t m_start{};
        mutable size_t m_end{};
        mutable size_t m_line{};
        mutable size_t m_column{};
        HasData m_has_data;

      public:
        RC_PB_API Token(int type, SystemStringViewType name, SystemStringViewType identifier, HasData has_data = HasData::No);

      public:
        RC_PB_API auto get_type() const -> int;
        RC_PB_API auto set_has_data(HasData) -> void;
        RC_PB_API auto has_data() const -> bool;
        RC_PB_API auto set_start(size_t) -> void;
        RC_PB_API auto get_start() const -> size_t;
        RC_PB_API auto set_end(size_t) -> void;
        RC_PB_API auto get_end() const -> size_t;
        RC_PB_API auto get_identifier() const -> SystemStringViewType;
        RC_PB_API auto get_line() const -> size_t;
        RC_PB_API auto get_column() const -> size_t;

        template <typename TokenRuleType>
        auto add_rule(std::shared_ptr<TokenRuleType>&& token_rule) -> void
        {
            m_rules.emplace_back(std::move(token_rule));
        }

        RC_PB_API auto get_rules() const -> const std::vector<std::shared_ptr<TokenRule>>&;
        [[nodiscard]] RC_PB_API auto to_string() const -> SystemStringType;

      public:
        template <typename TokenRuleType>
        auto static create_internal(Token& token) -> void
        {
            auto rule = std::make_shared<TokenRuleType>();
            token.add_rule(std::move(rule));
        }

        template <typename TokenRuleTypeOne, typename TokenRuleTypeTwo, typename... TokenRuleTypes>
        auto static create_internal(Token& token) -> void
        {
            auto rule = std::make_shared<TokenRuleTypeOne>();
            token.add_rule(std::move(rule));
            create_internal<TokenRuleTypeTwo, TokenRuleTypes...>(token);
        }

        RC_PB_API auto static create(int type, SystemStringViewType name, SystemStringViewType identifier, HasData = HasData::No) -> Token;

        template <typename TokenRuleType>
        auto static create(int type, SystemStringViewType name, SystemStringViewType identifier, HasData has_data = HasData::No) -> Token
        {
            Token token{type, name, identifier, has_data};
            create_internal<TokenRuleType>(token);
            return token;
        }

        template <typename TokenRuleTypeOne, typename TokenRuleTypeTwo, typename... TokenRuleTypes>
        auto static create(int type, SystemStringViewType name, SystemStringViewType identifier, HasData has_data = HasData::No) -> Token
        {
            Token token{type, name, identifier, has_data};
            create_internal<TokenRuleTypeOne, TokenRuleTypeTwo, TokenRuleTypes...>(token);
            return token;
        }
    };
} // namespace RC::ParserBase
