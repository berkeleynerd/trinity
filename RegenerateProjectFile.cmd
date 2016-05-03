@echo off
echo Checking out project and filters file
p4 edit TrinityAL.vcxproj
p4 edit TrinityAL.vcxproj.filters
p4 edit TrinityAL.orbis.vcxproj
p4 edit TrinityAL.orbis.vcxproj.filters
echo Regenerating
..\..\..\..\..\..\shared_tools\python\27\python.exe ..\..\tools\ProjectFileGenerator\ProjectFileGenerator.py -i TrinityAL.ccpproj
pause