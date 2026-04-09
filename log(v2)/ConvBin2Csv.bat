@echo %time%
@echo off
for %%f in (*.bin) do (
    echo log2csv.exe "%%f"
    log2csv.exe "%%f"
)
echo.
@echo %time%
echo Type "exit" to close this window.
cmd /k
