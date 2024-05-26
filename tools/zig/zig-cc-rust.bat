:; exec zig cc -target x86_64-linux-gnu "$@"; exit $?
@echo off
zig cc -target x86_64-linux-gnu %*