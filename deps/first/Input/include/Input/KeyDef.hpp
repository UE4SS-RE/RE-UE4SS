#pragma once

#include <stdexcept>
#include <cstdint>
#include <initializer_list>

namespace RC::Input
{
    static constexpr uint32_t max_callbacks_per_event = 30;
    static constexpr uint8_t max_keys = 0xFF;

    enum Key : uint8_t
    {
        RESERVED_START_OF_ENUM = 0x0,
        LEFT_MOUSE_BUTTON = 0x1,
        RIGHT_MOUSE_BUTTON = 0x2,
        CANCEL = 0x3,
        MIDDLE_MOUSE_BUTTON = 0x4,
        XBUTTON_ONE = 0x5,
        XBUTTON_TWO = 0x6,
        // UNDEFINED = 0x7,
        BACKSPACE = 0x8,
        TAB = 0x9,
        // UNDEFINED = 0x0A,
        // UNDEFINED = 0x0B,
        CLEAR = 0x0C,
        RETURN = 0x0D,
        // UNDEFINED = 0x0E,
        // UNDEFINED = 0x0F,
        // SHIFT = 0x10, // Modifier key
        // CONTROL = 0x11, // Modifier key
        // ALT = 0x12, // Modifier key
        PAUSE = 0x13,
        CAPS_LOCK = 0x14,
        IME_KANA = 0x15,
        IME_HANGUEL = 0x15,
        IME_HANGUL = 0x15,
        IME_ON = 0x16,
        IME_JUNJA = 0x17,
        IME_FINAL = 0x18,
        IME_HANJA = 0x19,
        IME_KANJI = 0x19,
        IME_OFF = 0x1A,
        ESCAPE = 0x1B,
        IME_CONVERT = 0x1C,
        IME_NONCONVERT = 0x1D,
        IME_ACCEPT = 0x1E,
        IME_MODECHANGE = 0x1F,
        SPACE = 0x20,
        PAGE_UP = 0x21,
        PAGE_DOWN = 0x22,
        END = 0x23,
        HOME = 0x24,
        LEFT_ARROW = 0x25,
        UP_ARROW = 0x26,
        RIGHT_ARROW = 0x27,
        DOWN_ARROW = 0x28,
        SELECT = 0x29,
        PRINT = 0x2A,
        EXECUTE = 0x2B,
        PRINT_SCREEN = 0x2C,
        INS = 0x2D,
        DEL = 0x2E,
        HELP = 0x2F,
        ZERO = 0x30,
        ONE = 0x31,
        TWO = 0x32,
        THREE = 0x33,
        FOUR = 0x34,
        FIVE = 0x35,
        SIX = 0x36,
        SEVEN = 0x37,
        EIGHT = 0x38,
        NINE = 0x39,
        // UNDEFINED = 0x3A,
        // UNDEFINED = 0x3B,
        // UNDEFINED = 0x3C,
        // UNDEFINED = 0x3D,
        // UNDEFINED = 0x3E,
        // UNDEFINED = 0x3F,
        // UNDEFINED = 0x40,
        A = 0x41,
        B = 0x42,
        C = 0x43,
        D = 0x44,
        E = 0x45,
        F = 0x46,
        G = 0x47,
        H = 0x48,
        I = 0x49,
        J = 0x4A,
        K = 0x4B,
        L = 0x4C,
        M = 0x4D,
        N = 0x4E,
        O = 0x4F,
        P = 0x50,
        Q = 0x51,
        R = 0x52,
        S = 0x53,
        T = 0x54,
        U = 0x55,
        V = 0x56,
        W = 0x57,
        X = 0x58,
        Y = 0x59,
        Z = 0x5A,
        LEFT_WIN = 0x5B,
        RIGHT_WIN = 0x5C,
        APPS = 0x5D,
        // RESERVED = 0x5E,
        SLEEP = 0x5F,
        NUM_ZERO = 0x60,
        NUM_ONE = 0x61,
        NUM_TWO = 0x62,
        NUM_THREE = 0x63,
        NUM_FOUR = 0x64,
        NUM_FIVE = 0x65,
        NUM_SIX = 0x66,
        NUM_SEVEN = 0x67,
        NUM_EIGHT = 0x68,
        NUM_NINE = 0x69,
        MULTIPLY = 0x6A,
        ADD = 0x6B,
        SEPARATOR = 0x6C,
        SUBTRACT = 0x6D,
        DECIMAL = 0x6E,
        DIVIDE = 0x6F,
        F1 = 0x70,
        F2 = 0x71,
        F3 = 0x72,
        F4 = 0x73,
        F5 = 0x74,
        F6 = 0x75,
        F7 = 0x76,
        F8 = 0x77,
        F9 = 0x78,
        F10 = 0x79,
        F11 = 0x7A,
        F12 = 0x7B,
        F13 = 0x7C,
        F14 = 0x7D,
        F15 = 0x7E,
        F16 = 0x7F,
        F17 = 0x80,
        F18 = 0x81,
        F19 = 0x82,
        F20 = 0x83,
        F21 = 0x84,
        F22 = 0x85,
        F23 = 0x86,
        F24 = 0x87,
        // UNDEFINED = 0x88,
        // UNDEFINED = 0x89,
        // UNDEFINED = 0x8A,
        // UNDEFINED = 0x8B,
        // UNDEFINED = 0x8C,
        // UNDEFINED = 0x8D,
        // UNDEFINED = 0x8E,
        // UNDEFINED = 0x8F,
        NUM_LOCK = 0x90,
        SCROLL_LOCK = 0x91,
        // OEM_SPECIFIC = 0x92,
        // OEM_SPECIFIC = 0x93,
        // OEM_SPECIFIC = 0x94,
        // OEM_SPECIFIC = 0x95,
        // OEM_SPECIFIC = 0x96,
        // UNDEFINED = 0x97,
        // UNDEFINED = 0x98,
        // UNDEFINED = 0x99,
        // UNDEFINED = 0x9A,
        // UNDEFINED = 0x9B,
        // UNDEFINED = 0x9C,
        // UNDEFINED = 0x9D,
        // UNDEFINED = 0x9E,
        // UNDEFINED = 0x9F,
        // LEFT_SHIFT = 0xA0, // Modifier key
        // RIGHT_SHIFT = 0xA1, // Modifier key
        // LEFT_CONTROL = 0xA2, // Modifier key
        // RIGHT_CONTROL = 0xA3, // Modifier key
        // LEFT_ALT = 0xA4, // Modifier key
        // RIGHT_ALT = 0xA5, // Modifier key
        BROWSER_BACK = 0xA6,
        BROWSER_FORWARD = 0xA7,
        BROWSER_REFRESH = 0xA8,
        BROWSER_STOP = 0xA9,
        BROWSER_SEARCH = 0xAA,
        BROWSER_FAVORITES = 0xAB,
        BROWSER_HOME = 0xAC,
        VOLUME_MUTE = 0xAD,
        VOLUME_DOWN = 0xAE,
        VOLUME_UP = 0xAF,
        MEDIA_NEXT_TRACK = 0xB0,
        MEDIA_PREV_TRACK = 0xB1,
        MEDIA_STOP = 0xB2,
        MEDIA_PLAY_PAUSE = 0xB3,
        LAUNCH_MAIL = 0xB4,
        LAUNCH_MEDIA_SELECT = 0xB5,
        LAUNCH_APP1 = 0xB6,
        LAUNCH_APP2 = 0xB7,
        // RESERVED = 0xB8,
        // RESERVED = 0xB9,
        OEM_ONE = 0xBA,
        OEM_PLUS = 0xBB,
        OEM_COMMA = 0xBC,
        OEM_MINUS = 0xBD,
        OEM_PERIOD = 0xBE,
        OEM_TWO = 0xBF,
        OEM_THREE = 0xC0,
        // RESERVED = 0xC1 - 0xD7,
        // UNDEFINED = 0xD8 - 0xDA,
        OEM_FOUR = 0xDB,
        OEM_FIVE = 0xDC,
        OEM_SIX = 0xDD,
        OEM_SEVEN = 0xDE,
        OEM_EIGHT = 0xDF,
        // RESERVED = 0xE0,
        // OEM_SPECIFIC = 0xE1,
        OEM_102 = 0xE2,
        // OEM_SPECIFIC = 0xE3 - 0xE4
        IME_PROCESS = 0xE5,
        // OEM_SPECIFIC = 0xE6,
        PACKET = 0xE7,
        // UNDEFINED = 0xE8,
        // OEM_SPECIFIC = 0xE9 - 0xF5,
        ATTN = 0xF6,
        CRSEL = 0xF7,
        EXSEL = 0xF8,
        EREOF = 0xF9,
        PLAY = 0xFA,
        ZOOM = 0xFB,
        NONAME = 0xFC, // RESERVED
        PA1 = 0xFD,
        OEM_CLEAR = 0xFE
    };

    enum ModifierKey : uint8_t
    {
        MOD_KEY_START_OF_ENUM = 0x0,

        SHIFT = 0x10,
        CONTROL = 0x11,
        ALT = 0x12,

        MODIFIER_KEYS_MAX,
    };

    static inline bool is_modify_key_valid(ModifierKey key)
    {
        return (key < MODIFIER_KEYS_MAX) && (key > MOD_KEY_START_OF_ENUM);
    }

    struct ModifierKeys
    {
        /// SAFETY: This is a bitfield, following static_assert ensures that the bitfield is not larger than 32 bits
        uint32_t keys;

        // allow ops between keys

        auto operator|(const ModifierKeys& key) -> ModifierKeys&;
        auto operator|=(const ModifierKeys& key) -> ModifierKeys&;
        auto operator|(const ModifierKey& key) -> ModifierKeys&;
        auto operator|=(const ModifierKey& key) -> ModifierKeys&;

        auto operator==(const ModifierKeys& key) const -> bool;
        auto operator!=(const ModifierKeys& key) const -> bool;

        auto operator<(const ModifierKeys& key) const -> bool;
        auto operator>(const ModifierKeys& key) const -> bool;

        ModifierKeys(const ModifierKey key) : keys{is_modify_key_valid(key) ? (1u << key) : 0} {};
        ModifierKeys(const ModifierKeys& other) : keys{other.keys} {};

        ModifierKeys() : keys{0} {};

        template <typename... Args>
        ModifierKeys(ModifierKey key, Args... args) : keys{(is_modify_key_valid(key) ? (1 << key) : 0) | ModifierKeys(args...).keys} {};

        ModifierKeys(std::initializer_list<ModifierKey> keys);

        template <typename TArray>
        ModifierKeys(const TArray& keys) : keys{0}
        {
            for (auto key : keys)
            {
                if (is_modify_key_valid(key))
                {
                    this->keys |= (1 << key);
                }
            }
        }

        bool empty() const
        {
            return keys == 0;
        }
    };

    static constexpr uint8_t max_modifier_keys = MODIFIER_KEYS_MAX;
    static_assert(max_modifier_keys < 32, "Modifier keys cannot exceed 32");

    auto operator&(const ModifierKeys& keys, const ModifierKey& key) -> bool;
    auto operator&(const ModifierKeys& keys, const ModifierKeys& key) -> bool;

    auto operator++(Input::Key& key) -> Input::Key&;
} // namespace RC::Input
