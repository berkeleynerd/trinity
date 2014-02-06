@echo off
del *.sb
del *.h
for %%i in (*.vs.pssl) do (
    orbis-psslc -profile sce_vs_vs_orbis -o %%~ni.sb  %%i
    orbis-psslc -profile sce_vs_vs_orbis -DPATCHED_SHADER=1 -o %%~ni.patched.sb  %%i
)
for %%i in (*.ps.pssl) do (
    orbis-psslc -profile sce_ps_orbis -o %%~ni.sb  %%i
    orbis-psslc -profile sce_ps_orbis -DPATCHED_SHADER=1 -o %%~ni.patched.sb  %%i
)
for %%i in (*.sb) do (
    @orbis-bin2h -q %%~ni.sb -o %%~ni.h
)
