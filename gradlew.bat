@echo off
rem CorvusRetro - CorvusSoftware
rem Copyright (C) 2026 CorvusSoftware
rem SPDX-License-Identifier: AGPL-3.0-only
pushd "%~dp0pkg\android"
call gradlew.bat %*
set corvus_exit=%errorlevel%
popd
exit /b %corvus_exit%
