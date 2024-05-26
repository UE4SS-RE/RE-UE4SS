#!/bin/bash
# 2>nul || @goto :batch
exec zig c++ -target x86_64-linux-gnu "$@"
goto :EOF

:batch
zig c++ -target x86_64-linux-gnu %*