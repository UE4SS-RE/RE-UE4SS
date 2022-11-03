@echo off

set CommitMessage=%~1
IF NOT DEFINED %CommitMessage (
    echo Could not commit and push changes because no commit message was given
    pause
    exit /b
)

cd Constructs
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd DynamicOutput
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd File
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd Function
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd FunctionTimer
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd Helpers
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd IniParser
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd Input
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd JSON
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd LuaMadeSimple
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd LuaRaw
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd MProgram
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd ParserBase
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd ScopedTimer
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd SinglePassSigScanner
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

cd Unreal
git add -A
git commit -m "%CommitMessage%"
git push
cd ..

pause