These templates are meant to be used as a base for games with custom engine versions or an otherwise unsupported engine version.

1. Figure out the engine version of your game.
2. Copy the template closest to that version to the same folder as the ue4ss.dll/xinput1_3.dll.
3. Rename the file to 'MemberVariableLayout.ini'.
4. Make your adjustments.
   Values are absolute offsets.
   Values can be hex if they are prefixed with 0x and entries with a value of -1 and entries that don't exist will use the default value for the detected engine version.
The game must be restarted after making any adjustments.