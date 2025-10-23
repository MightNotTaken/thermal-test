@echo off
chcp 65001 >nul
rmdir /s /q log
rmdir /s /q output
rmdir /s /q msvc\x64
echo clean up done!

echo start to build

mkdir log
cd msvc

echo compile Release_x64
devenv get_by_command.vcxproj /ReBuild "Release|x64" /project get_by_command.vcxproj -> ..\log\release_x64_log.txt && echo build compile sample succeeded! || echo compile target sample failed!

pause