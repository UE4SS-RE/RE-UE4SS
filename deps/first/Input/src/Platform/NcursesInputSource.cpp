#include <Input/Platform/NcursesInputSource.hpp>
#include <DynamicOutput/Output.hpp>

namespace RC::Input
{
    auto NcursesInputSource::begin_frame() -> void
    {
        std::fill(m_key_cur.begin(), m_key_cur.end(), false);
    }

    static Key translate_control_key(int k) {
        switch (k) {
            case 0: return Key::SPACE;
            case 1: return Key::A;
            case 2: return Key::B;
            case 3: return Key::C;
            case 4: return Key::D;
            case 5: return Key::E;
            case 6: return Key::F;
            case 7: return Key::G;
            case 8: return Key::H;
            case 9: return Key::I;
            case 10: return Key::J;
            case 11: return Key::K;
            case 12: return Key::L;
            case 13: return Key::M;
            case 14: return Key::N;
            case 15: return Key::O;
            case 16: return Key::P;
            case 17: return Key::Q;
            case 18: return Key::R;
            case 19: return Key::S;
            case 20: return Key::T;
            case 21: return Key::U;
            case 22: return Key::V;
            case 23: return Key::W;
            case 24: return Key::X;
            case 25: return Key::Y;
            case 26: return Key::Z;
            // case 27: return Key::Escape;
            case 28: return Key::BACKSPACE;
            //case 29: return Key::CtrlRightBracket;
            //case 30: return Key::CtrlCircumflex;
            // case 31: return Key::CtrlUnderscore;
            default: return Key::RESERVED_START_OF_ENUM;
        }
    }

    #define KEY_DOWN	0402		/* down-arrow key */
    #define KEY_UP		0403		/* up-arrow key */
    #define KEY_LEFT	0404		/* left-arrow key */
    #define KEY_RIGHT	0405		/* right-arrow key */
    #define KEY_HOME	0406		/* home key */
    #define KEY_BACKSPACE	0407		/* backspace key */
    #define KEY_F0		0410		/* Function keys.  Space for 64 */
    #define KEY_F(n)	(KEY_F0+(n))	/* Value of function key n */
    #define KEY_DC		0512		/* delete-character key */
    #define KEY_IC		0513		/* insert-character key */

    static Key translate_key_may_shift(int ch, bool &shifted) {
        shifted = false;
        if (ch >= 97 && ch <= 122) {
            return static_cast<Key>(ch - 97 + static_cast<int>(Key::A));
        }
        if (ch >= 65 && ch <= 90) {
            // capital letter
            shifted = true;
            return static_cast<Key>(ch - 65 + static_cast<int>(Key::A));
        }
        
        if (ch >= 48 && ch <= 57) {
            return static_cast<Key>(ch - 48 + static_cast<int>(Key::ZERO));
        }
        switch (ch) {
            case 32: return Key::SPACE;
            // case 10: return Key::ENTER;
            // case 27: return Key::ESCAPE;
            case 127: return Key::BACKSPACE;
            case KEY_DOWN: return Key::DOWN_ARROW;
            case KEY_UP: return Key::UP_ARROW;
            case KEY_LEFT: return Key::LEFT_ARROW;
            case KEY_RIGHT: return Key::RIGHT_ARROW;
            case KEY_HOME: return Key::HOME;
            case KEY_BACKSPACE: return Key::BACKSPACE;
            case KEY_F(1): return Key::F1;
            case KEY_F(2): return Key::F2;
            case KEY_F(3): return Key::F3;
            case KEY_F(4): return Key::F4;
            case KEY_F(5): return Key::F5;
            case KEY_F(6): return Key::F6;
            case KEY_F(7): return Key::F7;
            case KEY_F(8): return Key::F8;
            case KEY_F(9): return Key::F9;
            case KEY_F(10): return Key::F10;
            case KEY_F(11): return Key::F11;
            case KEY_F(12): return Key::F12;
            case KEY_DC: return Key::DEL;
            case KEY_IC: return Key::INS;
        }
        #define CASE_CHAR(normal_ch, shifted_ch, key) \
            case normal_ch: shifted = false; \
            case shifted_ch: return key;
        shifted = true;
        switch (ch) {
            case '!':
                return Key::ONE;
            case '@':
                return Key::TWO;
            case '#':
                return Key::THREE;
            case '$':
                return Key::FOUR;
            case '%':
                return Key::FIVE;
            case '^':
                return Key::SIX;
            case '&':
                return Key::SEVEN;
            case '*':
                return Key::EIGHT;
            case '(':
                return Key::NINE;
            case ')':
                return Key::ZERO; // Shift+0 produces ')'
            CASE_CHAR('`', '~', Key::OEM_THREE) // key ~ is oem 3 on windows
            CASE_CHAR('-', '_', Key::OEM_MINUS) // key _ is oem minus on windows
            CASE_CHAR('=', '+', Key::OEM_PLUS) // key + is oem plus on windows
            CASE_CHAR('[', '{', Key::OEM_FOUR) // key { is oem four on windows
            CASE_CHAR(']', '}', Key::OEM_SIX) // key } is oem 6 on windows
            CASE_CHAR('\\', '|', Key::OEM_FIVE) // key | is oem 5 on windows
            CASE_CHAR(';', ':', Key::OEM_ONE) // key : is oem semicolon on windows
            CASE_CHAR('\'', '"', Key::OEM_SEVEN) // key " is oem apostrophe on windows
            CASE_CHAR(',', '<', Key::OEM_COMMA) // key < is oem comma on windows
            CASE_CHAR('.', '>', Key::OEM_PERIOD) // key > is oem period on windows
            CASE_CHAR('/', '?', Key::OEM_TWO) // key ? is oem 2 on windows
            default:
                shifted = false;
                return Key::RESERVED_START_OF_ENUM;
        }
    }

    auto NcursesInputSource::receive_input(int input) -> void
    {
        if (!m_activated) {
            return;
        }

        // input is ncurse's key code
        if (input == 27) {
            // esc key
            m_is_alt_down = true;
        }
        else
        {
            auto control_down = false;
            auto shift_down = false;
            auto key_code = -1;
            if (input >= 0 && input < 32) {
                Key input_key = translate_control_key(input);
                if (input_key != Key::RESERVED_START_OF_ENUM) {
                    key_code = static_cast<int>(input_key);
                    control_down = true;
                }
            } else {
                Key input_key = translate_key_may_shift(input, shift_down);
                if (input_key != Key::RESERVED_START_OF_ENUM) 
                {
                    key_code = static_cast<int>(input_key);
                }
            }
            if (key_code != -1) {
                m_key_cur[key_code] = true;
                if (!m_key_last[key_code]) {
                    // keydown
                    ModifierKeys modifier_keys{};
                    modifier_keys |= (control_down ? ModifierKey::CONTROL : ModifierKey::MOD_KEY_START_OF_ENUM);
                    modifier_keys |= (shift_down ? ModifierKey::SHIFT : ModifierKey::MOD_KEY_START_OF_ENUM);
                    modifier_keys |= (m_is_alt_down ? ModifierKey::ALT : ModifierKey::MOD_KEY_START_OF_ENUM);
                    push_input_event({static_cast<Key>(key_code), modifier_keys});
                    auto modify_to_string = [](ModifierKeys modifier_keys) -> std::string {
                        std::string result;
                        if (modifier_keys & ModifierKey::CONTROL) {
                            result += "Control";
                        }
                        if (modifier_keys & ModifierKey::SHIFT) {
                            if (!result.empty()) {
                                result += "+";
                            }
                            result += "Shift";
                        }
                        if (modifier_keys & ModifierKey::ALT) {
                            if (!result.empty()) {
                                result += "+";
                            }
                            result += "Alt";
                        }
                        return result;
                    };
                    Output::send<LogLevel::Default>(SYSSTR("NcursesInputSource::receive_input: key down {} {}"), static_cast<int>(input), modify_to_string(modifier_keys));
                }
                else 
                {
                    Output::send<LogLevel::Default>(SYSSTR("NcursesInputSource::receive_input: key repeat {}"), static_cast<int>(input));
                }
            }
            m_is_alt_down = false;
        }
    }

    auto NcursesInputSource::end_frame() -> void
    {
        std::swap(m_key_cur, m_key_last);
    }

    auto NcursesInputSource::is_available() -> bool
    {
        return true;
    }
} // namespace RC::Input