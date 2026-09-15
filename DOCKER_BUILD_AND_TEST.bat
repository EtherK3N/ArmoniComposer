@echo off
title Armoni Composer - Docker Isolated Build & Test
echo =======================================================
echo   ARMONI COMPOSER - DOCKER ISOLATED TEST RUNNER
echo =======================================================
echo.

where docker >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Docker is not running or not installed in system PATH.
    echo Start Docker Desktop and try again.
    pause
    exit /b 1
)

echo [1/2] Building isolated Docker container image (k3n-armoni-tester)...
docker build -t k3n-armoni-tester .

if %errorlevel% neq 0 (
    echo [ERROR] Compilation error inside Docker container.
    pause
    exit /b 1
)

echo.
echo [2/2] Running automated test suite inside Docker...
echo.
docker run --rm k3n-armoni-tester

echo.
echo =======================================================
echo   TEST SUITE PASSED SUCCESSFULLY INSIDE DOCKER!
echo =======================================================
echo.
pause
