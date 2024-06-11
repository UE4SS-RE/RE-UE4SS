#include <Input/Platform/GLFW3InputSource.hpp>

#define GLFW_KEY_SPACE 32

#define GLFW_KEY_APOSTROPHE 39 /* ' */

#define GLFW_KEY_COMMA 44 /* , */

#define GLFW_KEY_MINUS 45 /* - */

#define GLFW_KEY_PERIOD 46 /* . */

#define GLFW_KEY_SLASH 47 /* / */

#define GLFW_KEY_0 48

#define GLFW_KEY_1 49

#define GLFW_KEY_2 50

#define GLFW_KEY_3 51

#define GLFW_KEY_4 52

#define GLFW_KEY_5 53

#define GLFW_KEY_6 54

#define GLFW_KEY_7 55

#define GLFW_KEY_8 56

#define GLFW_KEY_9 57

#define GLFW_KEY_SEMICOLON 59 /* ; */

#define GLFW_KEY_EQUAL 61 /* = */

#define GLFW_KEY_A 65

#define GLFW_KEY_B 66

#define GLFW_KEY_C 67

#define GLFW_KEY_D 68

#define GLFW_KEY_E 69

#define GLFW_KEY_F 70

#define GLFW_KEY_G 71

#define GLFW_KEY_H 72

#define GLFW_KEY_I 73

#define GLFW_KEY_J 74

#define GLFW_KEY_K 75

#define GLFW_KEY_L 76

#define GLFW_KEY_M 77

#define GLFW_KEY_N 78

#define GLFW_KEY_O 79

#define GLFW_KEY_P 80

#define GLFW_KEY_Q 81

#define GLFW_KEY_R 82

#define GLFW_KEY_S 83

#define GLFW_KEY_T 84

#define GLFW_KEY_U 85

#define GLFW_KEY_V 86

#define GLFW_KEY_W 87

#define GLFW_KEY_X 88

#define GLFW_KEY_Y 89

#define GLFW_KEY_Z 90

#define GLFW_KEY_LEFT_BRACKET 91 /* [ */

#define GLFW_KEY_BACKSLASH 92 /* \ */

#define GLFW_KEY_RIGHT_BRACKET 93 /* ] */

#define GLFW_KEY_GRAVE_ACCENT 96 /* ` */

#define GLFW_KEY_WORLD_1 161 /* non-US #1 */

#define GLFW_KEY_WORLD_2 162 /* non-US #2 */

#define GLFW_KEY_ESCAPE 256

#define GLFW_KEY_ENTER 257

#define GLFW_KEY_TAB 258

#define GLFW_KEY_BACKSPACE 259

#define GLFW_KEY_INSERT 260

#define GLFW_KEY_DELETE 261

#define GLFW_KEY_RIGHT 262

#define GLFW_KEY_LEFT 263

#define GLFW_KEY_DOWN 264

#define GLFW_KEY_UP 265

#define GLFW_KEY_PAGE_UP 266

#define GLFW_KEY_PAGE_DOWN 267

#define GLFW_KEY_HOME 268

#define GLFW_KEY_END 269

#define GLFW_KEY_CAPS_LOCK 280

#define GLFW_KEY_SCROLL_LOCK 281

#define GLFW_KEY_NUM_LOCK 282

#define GLFW_KEY_PRINT_SCREEN 283

#define GLFW_KEY_PAUSE 284

#define GLFW_KEY_F1 290

#define GLFW_KEY_F2 291

#define GLFW_KEY_F3 292

#define GLFW_KEY_F4 293

#define GLFW_KEY_F5 294

#define GLFW_KEY_F6 295

#define GLFW_KEY_F7 296

#define GLFW_KEY_F8 297

#define GLFW_KEY_F9 298

#define GLFW_KEY_F10 299

#define GLFW_KEY_F11 300

#define GLFW_KEY_F12 301

#define GLFW_KEY_F13 302

#define GLFW_KEY_F14 303

#define GLFW_KEY_F15 304

#define GLFW_KEY_F16 305

#define GLFW_KEY_F17 306

#define GLFW_KEY_F18 307

#define GLFW_KEY_F19 308

#define GLFW_KEY_F20 309

#define GLFW_KEY_F21 310

#define GLFW_KEY_F22 311

#define GLFW_KEY_F23 312

#define GLFW_KEY_F24 313

#define GLFW_KEY_F25 314

#define GLFW_KEY_KP_0 320

#define GLFW_KEY_KP_1 321

#define GLFW_KEY_KP_2 322

#define GLFW_KEY_KP_3 323

#define GLFW_KEY_KP_4 324

#define GLFW_KEY_KP_5 325

#define GLFW_KEY_KP_6 326

#define GLFW_KEY_KP_7 327

#define GLFW_KEY_KP_8 328

#define GLFW_KEY_KP_9 329

#define GLFW_KEY_KP_DECIMAL 330

#define GLFW_KEY_KP_DIVIDE 331

#define GLFW_KEY_KP_MULTIPLY 332

#define GLFW_KEY_KP_SUBTRACT 333

#define GLFW_KEY_KP_ADD 334

#define GLFW_KEY_KP_ENTER 335

#define GLFW_KEY_KP_EQUAL 336

#define GLFW_KEY_LEFT_SHIFT 340

#define GLFW_KEY_LEFT_CONTROL 341

#define GLFW_KEY_LEFT_ALT 342

#define GLFW_KEY_LEFT_SUPER 343

#define GLFW_KEY_RIGHT_SHIFT 344

#define GLFW_KEY_RIGHT_CONTROL 345

#define GLFW_KEY_RIGHT_ALT 346

#define GLFW_KEY_RIGHT_SUPER 347

#define GLFW_KEY_MENU 348

#define GLFW_KEY_LAST GLFW_KEY_MENU

#define GLFW_MOD_SHIFT 0x0001
#define GLFW_MOD_CONTROL 0x0002
#define GLFW_MOD_ALT 0x0004

#define GLFW_RELEASE 0
#define GLFW_PRESS 1
#define GLFW_REPEAT 2

namespace RC::Input
{
    auto GLFW3InputSource::begin_frame() -> void
    {
    }

    auto GLFW3InputSource::receive_input(int key, int action, int mods) -> void
    {
        auto modifier_keys = ModifierKeys{};
        modifier_keys |= (mods & GLFW_MOD_CONTROL ? ModifierKey::CONTROL : ModifierKey::MOD_KEY_START_OF_ENUM);
        modifier_keys |= (mods & GLFW_MOD_SHIFT ? ModifierKey::SHIFT : ModifierKey::MOD_KEY_START_OF_ENUM);
        modifier_keys |= (mods & GLFW_MOD_ALT ? ModifierKey::ALT : ModifierKey::MOD_KEY_START_OF_ENUM);
        if (action == GLFW_PRESS)
        {
            // translate key
            Key input = m_translate_key[key];
            if (input != RESERVED_START_OF_ENUM)
            {
                push_input_event({input, modifier_keys});
            }
        }
    }

    auto GLFW3InputSource::end_frame() -> void
    {
    }

    auto GLFW3InputSource::is_available() -> bool
    {
        return true;
    }

    GLFW3InputSource::GLFW3InputSource()
    {
        // init to 0
        for (int i = 0; i < 512; ++i)
        {
            m_translate_key[i] = static_cast<Key>(0);
        }
        // Alphanumeric keys
        for (int key = GLFW_KEY_A; key <= GLFW_KEY_Z; ++key)
        {
            m_translate_key[key] = static_cast<Key>(key);
        }
        for (int key = GLFW_KEY_0; key <= GLFW_KEY_9; ++key)
        {
            m_translate_key[key] = static_cast<Key>(key + 22);
        }

        // Symbol keys
        m_translate_key[GLFW_KEY_SPACE] = Key::SPACE;
        m_translate_key[GLFW_KEY_APOSTROPHE] = Key::OEM_SEVEN;
        m_translate_key[GLFW_KEY_COMMA] = Key::OEM_COMMA;
        m_translate_key[GLFW_KEY_MINUS] = Key::OEM_MINUS;
        m_translate_key[GLFW_KEY_PERIOD] = Key::OEM_PERIOD;
        m_translate_key[GLFW_KEY_SLASH] = Key::OEM_TWO;
        m_translate_key[GLFW_KEY_SEMICOLON] = Key::OEM_ONE;
        m_translate_key[GLFW_KEY_EQUAL] = Key::OEM_PLUS;
        m_translate_key[GLFW_KEY_LEFT_BRACKET] = Key::OEM_FOUR;
        m_translate_key[GLFW_KEY_BACKSLASH] = Key::OEM_FIVE;
        m_translate_key[GLFW_KEY_RIGHT_BRACKET] = Key::OEM_SIX;
        m_translate_key[GLFW_KEY_GRAVE_ACCENT] = Key::OEM_THREE;

        // Control keys
        m_translate_key[GLFW_KEY_ESCAPE] = Key::ESCAPE;
        m_translate_key[GLFW_KEY_ENTER] = Key::RETURN;
        m_translate_key[GLFW_KEY_TAB] = Key::TAB;
        m_translate_key[GLFW_KEY_BACKSPACE] = Key::BACKSPACE;
        m_translate_key[GLFW_KEY_INSERT] = Key::INS;
        m_translate_key[GLFW_KEY_DELETE] = Key::DEL;
        m_translate_key[GLFW_KEY_RIGHT] = Key::RIGHT_ARROW;
        m_translate_key[GLFW_KEY_LEFT] = Key::LEFT_ARROW;
        m_translate_key[GLFW_KEY_DOWN] = Key::DOWN_ARROW;
        m_translate_key[GLFW_KEY_UP] = Key::UP_ARROW;
        m_translate_key[GLFW_KEY_PAGE_UP] = Key::PAGE_UP;
        m_translate_key[GLFW_KEY_PAGE_DOWN] = Key::PAGE_DOWN;
        m_translate_key[GLFW_KEY_HOME] = Key::HOME;
        m_translate_key[GLFW_KEY_END] = Key::END;
        m_translate_key[GLFW_KEY_CAPS_LOCK] = Key::CAPS_LOCK;
        m_translate_key[GLFW_KEY_SCROLL_LOCK] = Key::SCROLL_LOCK;
        m_translate_key[GLFW_KEY_NUM_LOCK] = Key::NUM_LOCK;
        m_translate_key[GLFW_KEY_PRINT_SCREEN] = Key::PRINT_SCREEN;
        m_translate_key[GLFW_KEY_PAUSE] = Key::PAUSE;

        // Function keys
        for (int key = GLFW_KEY_F1; key <= GLFW_KEY_F25; ++key)
        {
            m_translate_key[key] = static_cast<Key>(key + 111);
        }

        // Numeric keypad
        for (int key = GLFW_KEY_KP_0; key <= GLFW_KEY_KP_EQUAL; ++key)
        {
            m_translate_key[key] = static_cast<Key>(key + 208);
        }
    }
} // namespace RC::Input