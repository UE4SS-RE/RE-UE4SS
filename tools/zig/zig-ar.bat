#!/bin/bash
# 2>nul || @goto :batch
exec zig ar "$@"
goto :EOF

:batch
zig ar %*