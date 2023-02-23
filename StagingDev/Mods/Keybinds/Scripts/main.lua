--[[
    This file contains bindings for all built-in tools.
    If you have a legacy bindings mod (listed below) then the binding in that mod takes precedence.

    List of legacy bindings mods:
    ObjectDumperMod
    CXXHeaderGeneratorMod
    UHTCompatibleHeaderGeneratorMod

    You can modify the configuration below if you're not happy with the default key bindings.
    Valid keys and modifier keys can be found at the bottom of this file.
--]]
Keybinds = {
    ["ObjectDumper"]                 = {["Key"] = Key.J,             ["ModifierKeys"] = {}},
    ["CXXHeaderGenerator"]           = {["Key"] = Key.D,             ["ModifierKeys"] = {ModifierKey.CONTROL}},
    ["UHTCompatibleHeaderGenerator"] = {["Key"] = Key.NUM_NINE,      ["ModifierKeys"] = {ModifierKey.CONTROL}},
    ["DumpStaticMeshes"]             = {["Key"] = Key.NUM_EIGHT,     ["ModifierKeys"] = {ModifierKey.CONTROL}},
    ["DumpAllActors"]                = {["Key"] = Key.NUM_SEVEN,     ["ModifierKeys"] = {ModifierKey.CONTROL}},
    ["DumpUSMAP"]                    = {["Key"] = Key.NUM_SIX,       ["ModifierKeys"] = {ModifierKey.CONTROL}},
}

-- Logic, DO NOT CHANGE!
local function RegisterKey(KeyBindName, Callable)
    if (Keybinds[KeyBindName] and not IsKeyBindRegistered(Keybinds[KeyBindName].Key, Keybinds[KeyBindName].ModifierKeys)) then
        RegisterKeyBind(Keybinds[KeyBindName].Key, Keybinds[KeyBindName].ModifierKeys, Callable)
    end
end

RegisterKey("ObjectDumper", function() DumpAllObjects() end)
RegisterKey("CXXHeaderGenerator", function() GenerateSDK() end)
RegisterKey("UHTCompatibleHeaderGenerator", function() GenerateUHTCompatibleHeaders() end)
RegisterKey("DumpStaticMeshes", function() DumpStaticMeshes() end)
RegisterKey("DumpAllActors", function() DumpAllActors() end)
RegisterKey("DumpUSMAP", function() DumpUSMAP() end)

--[[
    Valid keys:
    LEFT_MOUSE_BUTTON
    RIGHT_MOUSE_BUTTON
    CANCEL
    MIDDLE_MOUSE_BUTTON
    XBUTTON_ONE
    XBUTTON_TWO
    BACKSPACE
    TAB
    CLEAR
    RETURN
    PAUSE
    CAPS_LOCK
    IME_KANA
    IME_HANGUEL
    IME_HANGUL
    IME_ON
    IME_JUNJA
    IME_FINAL
    IME_HANJA
    IME_KANJI
    IME_OFF
    ESCAPE
    IME_CONVERT
    IME_NONCONVERT
    IME_ACCEPT
    IME_MODECHANGE
    SPACE
    PAGE_UP
    PAGE_DOWN
    END
    HOME
    LEFT_ARROW
    UP_ARROW
    RIGHT_ARROW
    DOWN_ARROW
    SELECT
    PRINT
    EXECUTE
    PRINT_SCREEN
    INS
    DEL
    HELP
    ZERO
    ONE
    TWO
    THREE
    FOUR
    FIVE
    SIX
    SEVEN
    EIGHT
    NINE
    A
    B
    C
    D
    E
    F
    G
    H
    I
    J
    K
    L
    M
    N
    O
    P
    Q
    R
    S
    T
    U
    V
    W
    X
    Y
    Z
    LEFT_WIN
    RIGHT_WIN
    APPS
    SLEEP
    NUM_ZERO
    NUM_ONE
    NUM_TWO
    NUM_THREE
    NUM_FOUR
    NUM_FIVE
    NUM_SIX
    NUM_SEVEN
    NUM_EIGHT
    NUM_NINE
    MULTIPLY
    ADD
    SEPARATOR
    SUBTRACT
    DECIMAL
    DIVIDE
    F1
    F2
    F3
    F4
    F5
    F6
    F7
    F8
    F9
    F10
    F11
    F12
    F13
    F14
    F15
    F16
    F17
    F18
    F19
    F20
    F21
    F22
    F23
    F24
    NUM_LOCK
    SCROLL_LOCK
    BROWSER_BACK
    BROWSER_FORWARD
    BROWSER_REFRESH
    BROWSER_STOP
    BROWSER_SEARCH
    BROWSER_FAVORITES
    BROWSER_HOME
    VOLUME_MUTE
    VOLUME_DOWN
    VOLUME_UP
    MEDIA_NEXT_TRACK
    MEDIA_PREV_TRACK
    MEDIA_STOP
    MEDIA_PLAY_PAUSE
    LAUNCH_MAIL
    LAUNCH_MEDIA_SELECT
    LAUNCH_APP1
    LAUNCH_APP2
    OEM_ONE
    OEM_PLUS
    OEM_COMMA
    OEM_MINUS
    OEM_PERIOD
    OEM_TWO
    OEM_THREE
    OEM_FOUR
    OEM_FIVE
    OEM_SIX
    OEM_SEVEN
    OEM_EIGHT
    OEM_102
    IME_PROCESS
    PACKET
    ATTN
    CRSEL
    EXSEL
    EREOF
    PLAY
    ZOOM
    PA1
    OEM_CLEAR

    Valid modifier keys:
    SHIFT
    CONTROL
    ALT
--]]
