rem =======================================================
rem     Make DLL from Static Library for WIN32
rem Copyright (C) 2010-2011 Zilong Tan <eric.zltan@gmail.com>
rem =======================================================

@echo off

@cls

@if exist *.o del *.o

@for /R %%a in (*.a) do echo Extracting %%a ... && ar x %%a

@echo Building OUTPUT.DLL ...

@dllwrap -o OUTPUT.DLL --export-all-symbols *.o
@if errorlevel 1 goto dll_error

@del *.o

goto done

:dll_error
@echo "DLLWRAP failure."
@goto done

:done
@pause
