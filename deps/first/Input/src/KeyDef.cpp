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
}
