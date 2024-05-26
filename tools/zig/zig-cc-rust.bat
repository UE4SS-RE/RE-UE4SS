#!/bin/bash
# 2>nul || @goto :batch
exec zig cc -target x86_64-linux-gnu "$@"
goto :EOF

:batch
zig cc -target x86_64-linux-gnu %*