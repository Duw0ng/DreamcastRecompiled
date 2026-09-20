@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
call build_windows.bat --no-pause
if errorlevel 1 goto :error

call run_multifunction_test.bat
if errorlevel 1 goto :error

call run_branches_test.bat
if errorlevel 1 goto :error

call run_memory_test.bat
if errorlevel 1 goto :error

call run_structs_test.bat
if errorlevel 1 goto :error

call run_addressbus_test.bat
if errorlevel 1 goto :error

call run_not_test.bat
if errorlevel 1 goto :error

call run_cpu_batch_test.bat
if errorlevel 1 goto :error

call run_cpu_control_test.bat
if errorlevel 1 goto :error

call run_fpu_moves_test.bat
if errorlevel 1 goto :error

call run_fpu_arith_test.bat
if errorlevel 1 goto :error

call run_fpu_unary_test.bat
if errorlevel 1 goto :error

call run_system_control_test.bat
if errorlevel 1 goto :error

call run_rte_test.bat
if errorlevel 1 goto :error

echo.
echo [OK] Suite CTest + multi-funcion + branches + memoria + structs/bitops + address-bus + CPU batch + CPU control + FPU moves + FPU arithmetic + FPU unary/vector + system/control + RTE completados.
echo      Los ELF reales de KallistiOS se prueban aparte:
echo        run_native_hello.bat
echo        run_kos_memtest_databus.bat
echo        run_kos_memtest_addressbus.bat
echo        run_kos_memtest_device.bat
echo        run_kos_ieee_sqrtf.bat
echo        run_kos_arch_tls.bat
pause
exit /b 0

:error
echo.
echo [ERROR] La suite completa fallo.
pause
exit /b 1
