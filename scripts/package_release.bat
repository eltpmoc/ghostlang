@echo off
rem Empacotador da release portatil: a logica fica em package_release.ps1.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0package_release.ps1" %*
