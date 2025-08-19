#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>

#include <Input/KeyDef.hpp>
#include <Helpers/String.hpp>
#include <fmt/core.h>

namespace RC::Input
{
    static std::unordered_map<StringType, Key> lowercase_string_to_key{{STR("reserved_start_of_enum"), Key::RESERVED_START_OF_ENUM},
                                                                       {STR("left_mouse_button"), Key::LEFT_MOUSE_BUTTON},
                                                                       {STR("right_mouse_button"), Key::RIGHT_MOUSE_BUTTON},
                                                                       {STR("cancel"), Key::CANCEL},
                                                                       {STR("middle_mouse_button"), Key::MIDDLE_MOUSE_BUTTON},
                                                                       {STR("xbutton_one"), Key::XBUTTON_ONE},
                                                                       {STR("xbutton_two"), Key::XBUTTON_TWO},
                                                                       {STR("backspace"), Key::BACKSPACE},
                                                                       {STR("tab"), Key::TAB},
                                                                       {STR("clear"), Key::CLEAR},
                                                                       {STR("return"), Key::RETURN},
                                                                       {STR("pause"), Key::PAUSE},
                                                                       {STR("caps_lock"), Key::CAPS_LOCK},
                                                                       {STR("ime_kana"), Key::IME_KANA},
                                                                       {STR("ime_hanguel"), Key::IME_HANGUEL},
                                                                       {STR("ime_hangul"), Key::IME_HANGUL},
                                                                       {STR("ime_on"), Key::IME_ON},
                                                                       {STR("ime_junja"), Key::IME_JUNJA},
                                                                       {STR("ime_final"), Key::IME_FINAL},
                                                                       {STR("ime_hanja"), Key::IME_HANJA},
                                                                       {STR("ime_kanji"), Key::IME_KANJI},
                                                                       {STR("ime_off"), Key::IME_OFF},
                                                                       {STR("escape"), Key::ESCAPE},
                                                                       {STR("ime_convert"), Key::IME_CONVERT},
                                                                       {STR("ime_nonconvert"), Key::IME_NONCONVERT},
                                                                       {STR("ime_accept"), Key::IME_ACCEPT},
                                                                       {STR("ime_modechange"), Key::IME_MODECHANGE},
                                                                       {STR("space"), Key::SPACE},
                                                                       {STR("page_up"), Key::PAGE_UP},
                                                                       {STR("page_down"), Key::PAGE_DOWN},
                                                                       {STR("end"), Key::END},
                                                                       {STR("home"), Key::HOME},
                                                                       {STR("left_arrow"), Key::LEFT_ARROW},
                                                                       {STR("up_arrow"), Key::UP_ARROW},
                                                                       {STR("right_arrow"), Key::RIGHT_ARROW},
                                                                       {STR("down_arrow"), Key::DOWN_ARROW},
                                                                       {STR("select"), Key::SELECT},
                                                                       {STR("print"), Key::PRINT},
                                                                       {STR("execute"), Key::EXECUTE},
                                                                       {STR("print_screen"), Key::PRINT_SCREEN},
                                                                       {STR("ins"), Key::INS},
                                                                       {STR("del"), Key::DEL},
                                                                       {STR("help"), Key::HELP},
                                                                       {STR("zero"), Key::ZERO},
                                                                       {STR("one"), Key::ONE},
                                                                       {STR("two"), Key::TWO},
                                                                       {STR("three"), Key::THREE},
                                                                       {STR("four"), Key::FOUR},
                                                                       {STR("five"), Key::FIVE},
                                                                       {STR("six"), Key::SIX},
                                                                       {STR("seven"), Key::SEVEN},
                                                                       {STR("eight"), Key::EIGHT},
                                                                       {STR("nine"), Key::NINE},
                                                                       {STR("a"), Key::A},
                                                                       {STR("b"), Key::B},
                                                                       {STR("c"), Key::C},
                                                                       {STR("d"), Key::D},
                                                                       {STR("e"), Key::E},
                                                                       {STR("f"), Key::F},
                                                                       {STR("g"), Key::G},
                                                                       {STR("h"), Key::H},
                                                                       {STR("i"), Key::I},
                                                                       {STR("j"), Key::J},
                                                                       {STR("k"), Key::K},
                                                                       {STR("l"), Key::L},
                                                                       {STR("m"), Key::M},
                                                                       {STR("n"), Key::N},
                                                                       {STR("o"), Key::O},
                                                                       {STR("p"), Key::P},
                                                                       {STR("q"), Key::Q},
                                                                       {STR("r"), Key::R},
                                                                       {STR("s"), Key::S},
                                                                       {STR("t"), Key::T},
                                                                       {STR("u"), Key::U},
                                                                       {STR("v"), Key::V},
                                                                       {STR("w"), Key::W},
                                                                       {STR("x"), Key::X},
                                                                       {STR("y"), Key::Y},
                                                                       {STR("z"), Key::Z},
                                                                       {STR("left_win"), Key::LEFT_WIN},
                                                                       {STR("right_win"), Key::RIGHT_WIN},
                                                                       {STR("apps"), Key::APPS},
                                                                       {STR("sleep"), Key::SLEEP},
                                                                       {STR("num_zero"), Key::NUM_ZERO},
                                                                       {STR("num_one"), Key::NUM_ONE},
                                                                       {STR("num_two"), Key::NUM_TWO},
                                                                       {STR("num_three"), Key::NUM_THREE},
                                                                       {STR("num_four"), Key::NUM_FOUR},
                                                                       {STR("num_five"), Key::NUM_FIVE},
                                                                       {STR("num_six"), Key::NUM_SIX},
                                                                       {STR("num_seven"), Key::NUM_SEVEN},
                                                                       {STR("num_eight"), Key::NUM_EIGHT},
                                                                       {STR("num_nine"), Key::NUM_NINE},
                                                                       {STR("multiply"), Key::MULTIPLY},
                                                                       {STR("add"), Key::ADD},
                                                                       {STR("separator"), Key::SEPARATOR},
                                                                       {STR("subtract"), Key::SUBTRACT},
                                                                       {STR("decimal"), Key::DECIMAL},
                                                                       {STR("divide"), Key::DIVIDE},
                                                                       {STR("f1"), Key::F1},
                                                                       {STR("f2"), Key::F2},
                                                                       {STR("f3"), Key::F3},
                                                                       {STR("f4"), Key::F4},
                                                                       {STR("f5"), Key::F5},
                                                                       {STR("f6"), Key::F6},
                                                                       {STR("f7"), Key::F7},
                                                                       {STR("f8"), Key::F8},
                                                                       {STR("f9"), Key::F9},
                                                                       {STR("f10"), Key::F10},
                                                                       {STR("f11"), Key::F11},
                                                                       {STR("f12"), Key::F12},
                                                                       {STR("f13"), Key::F13},
                                                                       {STR("f14"), Key::F14},
                                                                       {STR("f15"), Key::F15},
                                                                       {STR("f16"), Key::F16},
                                                                       {STR("f17"), Key::F17},
                                                                       {STR("f18"), Key::F18},
                                                                       {STR("f19"), Key::F19},
                                                                       {STR("f20"), Key::F20},
                                                                       {STR("f21"), Key::F21},
                                                                       {STR("f22"), Key::F22},
                                                                       {STR("f23"), Key::F23},
                                                                       {STR("f24"), Key::F24},
                                                                       {STR("num_lock"), Key::NUM_LOCK},
                                                                       {STR("scroll_lock"), Key::SCROLL_LOCK},
                                                                       {STR("browser_back"), Key::BROWSER_BACK},
                                                                       {STR("browser_forward"), Key::BROWSER_FORWARD},
                                                                       {STR("browser_refresh"), Key::BROWSER_REFRESH},
                                                                       {STR("browser_stop"), Key::BROWSER_STOP},
                                                                       {STR("browser_search"), Key::BROWSER_SEARCH},
                                                                       {STR("browser_favorites"), Key::BROWSER_FAVORITES},
                                                                       {STR("browser_home"), Key::BROWSER_HOME},
                                                                       {STR("volume_mute"), Key::VOLUME_MUTE},
                                                                       {STR("volume_down"), Key::VOLUME_DOWN},
                                                                       {STR("volume_up"), Key::VOLUME_UP},
                                                                       {STR("media_next_track"), Key::MEDIA_NEXT_TRACK},
                                                                       {STR("media_prev_track"), Key::MEDIA_PREV_TRACK},
                                                                       {STR("media_stop"), Key::MEDIA_STOP},
                                                                       {STR("media_play_pause"), Key::MEDIA_PLAY_PAUSE},
                                                                       {STR("launch_mail"), Key::LAUNCH_MAIL},
                                                                       {STR("launch_media_select"), Key::LAUNCH_MEDIA_SELECT},
                                                                       {STR("launch_app1"), Key::LAUNCH_APP1},
                                                                       {STR("launch_app2"), Key::LAUNCH_APP2},
                                                                       {STR("oem_one"), Key::OEM_ONE},
                                                                       {STR("oem_plus"), Key::OEM_PLUS},
                                                                       {STR("oem_comma"), Key::OEM_COMMA},
                                                                       {STR("oem_minus"), Key::OEM_MINUS},
                                                                       {STR("oem_period"), Key::OEM_PERIOD},
                                                                       {STR("oem_two"), Key::OEM_TWO},
                                                                       {STR("oem_three"), Key::OEM_THREE},
                                                                       {STR("oem_four"), Key::OEM_FOUR},
                                                                       {STR("oem_five"), Key::OEM_FIVE},
                                                                       {STR("oem_six"), Key::OEM_SIX},
                                                                       {STR("oem_seven"), Key::OEM_SEVEN},
                                                                       {STR("oem_eight"), Key::OEM_EIGHT},
                                                                       {STR("oem_102"), Key::OEM_102},
                                                                       {STR("ime_process"), Key::IME_PROCESS},
                                                                       {STR("packet"), Key::PACKET},
                                                                       {STR("attn"), Key::ATTN},
                                                                       {STR("crsel"), Key::CRSEL},
                                                                       {STR("exsel"), Key::EXSEL},
                                                                       {STR("ereof"), Key::EREOF},
                                                                       {STR("play"), Key::PLAY},
                                                                       {STR("zoom"), Key::ZOOM},
                                                                       {STR("noname"), Key::NONAME},
                                                                       {STR("pa1"), Key::PA1},
                                                                       {STR("oem_clear"), Key::OEM_CLEAR}};

    auto string_to_key(const StringType& string) -> Key
    {
        auto lowercase_string = string;
        std::ranges::transform(lowercase_string, lowercase_string.begin(), [](CharType c) {
            if constexpr (sizeof(c) > 1)
            {
                return std::towlower(c);
            }
            else
            {
                return std::tolower(c);
            }
        });
        if (const auto it = lowercase_string_to_key.find(lowercase_string); it == lowercase_string_to_key.end())
        {
            throw std::runtime_error{fmt::format("string_to_key: Key not found: {}", to_string(string))};
        }
        else
        {
            return it->second;
        }
    }

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
