@echo off
echo Checking out project and filters file
p4 edit TrinityALTest.vcxproj
p4 edit TrinityALTest.vcxproj.filters
echo Regenerating
..\..\..\..\..\..\shared_tools\python\27\python.exe ..\..\tools\ProjectFileGenerator\ProjectFileGenerator.py -i TrinityALTest.ccpproj
pause