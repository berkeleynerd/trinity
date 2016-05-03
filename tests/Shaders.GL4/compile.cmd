@echo off
del *.h
for %%i in (*.vsh) do (
    @orbis-bin2h -q %%i -o %%~ni.vs.h
    @orbis-bin2h -q %%i -o %%~ni.vs.patched.h
)
for %%i in (*.psh) do (
    @orbis-bin2h -q %%i -o %%~ni.ps.h
    @orbis-bin2h -q %%i -o %%~ni.ps.patched.h
)