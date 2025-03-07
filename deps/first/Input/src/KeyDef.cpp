#include <cstdint>

#include <Input/KeyDef.hpp>

namespace RC::Input
{
    auto operator++(Input::Key& key) -> Input::Key&
    {
        uint8_t next_key = static_cast<uint8_t>(key) + 1;
        if (next_key > max_keys)
        {
            throw std::runtime_error{"[Input::Key& operator++(Input::Key& key)] There was no next key, overflow error"};
        }

        key = static_cast<Input::Key>(next_key);

        return key;
    }

    auto ModifierKeys::operator|(const ModifierKeys& rkeys) -> ModifierKeys&
    {
        keys |= rkeys.keys;
        return *this;
    }

    auto ModifierKeys::operator|(const ModifierKey& key) -> ModifierKeys&
    {
        if (is_modify_key_valid(key))
        {
            keys |= (1 << key);
        }
        return *this;
    }

    auto ModifierKeys::operator|=(const ModifierKeys& rkeys) -> ModifierKeys&
    {
        return *this = *this | rkeys;
    }

    auto ModifierKeys::operator|=(const ModifierKey& key) -> ModifierKeys&
    {
        return *this = *this | key;
    }

    auto ModifierKeys::operator==(const ModifierKeys& key) const -> bool
    {
        return keys == key.keys;
    }

    auto ModifierKeys::operator<(const ModifierKeys& rkeys) const -> bool
    {
        return keys < rkeys.keys;
    }

    auto ModifierKeys::operator>(const ModifierKeys& rkeys) const -> bool
    {
        return keys > rkeys.keys;
    }

    auto ModifierKeys::operator!=(const ModifierKeys& key) const -> bool
    {
        return keys != key.keys;
    }

    ModifierKeys::ModifierKeys(std::initializer_list<ModifierKey> keys)
    {
        for (auto key : keys)
        {
            if (is_modify_key_valid(key))
            {
                this->keys |= (1 << key);
            }
        }
    }

    auto operator&(const ModifierKeys& keys, const ModifierKey& key) -> bool
    {
        return !!(keys.keys & (1 << key));
    }

    auto operator&(const ModifierKeys& keys, const ModifierKeys& key) -> bool
    {
        return !!(keys.keys & key.keys);
    }

} // namespace RC::Input
