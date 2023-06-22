@echo off

call internal_generate_build_files.bat Release
MSBuild.exe /m "VS_Solution\ue4ss.vcxproj" /property:Configuration=Release

call internal_generate_build_files.bat Release
MSBuild.exe /m "VS_Solution\xinput1_3.vcxproj" /property:Configuration=Release
