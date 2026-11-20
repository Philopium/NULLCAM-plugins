@echo off
setlocal EnableExtensions
cd /d "%~dp0"

echo ============================================
echo  MemoryGranulatorMode - build VST3 per Windows
echo ============================================
echo.

where cmake >nul 2>nul
if errorlevel 1 (
    echo ERRORE: cmake non e' nel PATH.
    echo Installa CMake 3.22 o successivo e riapri questa finestra.
    pause
    exit /b 1
)

where git >nul 2>nul
if errorlevel 1 (
    echo ERRORE: git non trovato. JUCE viene scaricato da GitHub, quindi serve.
    echo Installa Git e riapri questa finestra.
    pause
    exit /b 1
)

set "JUCE_OPTION="
if not "%~1"=="" set "JUCE_OPTION=-DJUCE_DIR=%~1"

echo [1/8] Configurazione...
if defined JUCE_OPTION (
    cmake -B build -G "Visual Studio 17 2022" -A x64 "%JUCE_OPTION%"
) else (
    cmake -B build -G "Visual Studio 17 2022" -A x64
)
if errorlevel 1 (
    echo.
    echo Configurazione fallita. Cause piu' comuni:
    echo  - manca Visual Studio con il carico di lavoro "Sviluppo di applicazioni desktop con C++"
    echo  - il generatore non e' VS 2022: prova a togliere  -G "Visual Studio 17 2022" -A x64
    echo  - JUCE non e' stato scaricato: controlla la connessione e riprova
    pause
    exit /b 1
)

echo.
echo [2/8] Compilazione VST3 + Standalone...
cmake --build build --config Release --target MemoryGranulatorMode_VST3 MemoryGranulatorMode_Standalone --parallel
if errorlevel 1 (
    echo.
    echo Compilazione fallita: copia le ultime righe di errore.
    pause
    exit /b 1
)

set "SRC=build\MemoryGranulatorMode_artefacts\Release\VST3\MemoryGranulatorMode.vst3"
if not exist "%SRC%" (
    echo ERRORE: non trovo "%SRC%"
    pause
    exit /b 1
)

echo.
echo [3/8] Test del motore (non serve una DAW)...
cmake --build build --config Release --target memorygranulator_dsp_test
if errorlevel 1 (
    echo I test del DSP non sono stati compilati.
    pause
    exit /b 1
)
build\Release\memorygranulator_dsp_test.exe
if errorlevel 1 (
    echo.
    echo I test del DSP sono falliti: guarda il messaggio sopra.
    pause
    exit /b 1
)

echo.
echo [4/8] Render del pannello + test dell'interfaccia...
cmake --build build --config Release --target memorygranulator_ui_check
if errorlevel 1 (
    echo Il test dell'interfaccia non e' stato compilato.
    pause
    exit /b 1
)
if not exist docs mkdir docs
build\memorygranulator_ui_check_artefacts\Release\memorygranulator_ui_check.exe docs\panel.png 1
if errorlevel 1 (
    echo Il render 1x e il test dell'interfaccia sono falliti.
    pause
    exit /b 1
)
build\memorygranulator_ui_check_artefacts\Release\memorygranulator_ui_check.exe docs\panel_2x.png 2
if errorlevel 1 (
    echo Il render 2x e il test dell'interfaccia sono falliti.
    pause
    exit /b 1
)

echo.
echo [5/8] Prova a caricare il VST3 costruito, come fa una DAW...
cmake --build build --config Release --target memorygranulator_vst3_check
if errorlevel 1 (
    echo Il test VST3 non e' stato compilato.
    pause
    exit /b 1
)
build\memorygranulator_vst3_check_artefacts\Release\memorygranulator_vst3_check.exe
if errorlevel 1 (
    echo.
    echo Il VST3 non si carica: guarda il messaggio sopra.
    pause
    exit /b 1
)

echo.
echo [6/8] Verifica che tutti i 99 parametri siano realmente collegati...
cmake --build build --config Release --target memorygranulator_parameter_coverage_check
if errorlevel 1 (
    echo.
    echo La verifica di copertura dei parametri e' fallita.
    pause
    exit /b 1
)

echo.
echo [7/8] Preparo lo zip da mandare...
if not exist dist mkdir dist
if exist "dist\MemoryGranulatorMode.vst3" rmdir /s /q "dist\MemoryGranulatorMode.vst3"
xcopy /E /I /Y "%SRC%" "dist\MemoryGranulatorMode.vst3" >nul
if errorlevel 1 (
    echo ERRORE: non riesco a copiare il bundle nella cartella dist.
    pause
    exit /b 1
)
copy /Y "INSTALLAZIONE.md" "dist\LEGGIMI - installazione.md" >nul
copy /Y "..\LICENSE.md" "dist\LICENZA - AGPLv3.md" >nul
if exist "MemoryGranulatorMode-Windows-VST3.zip" del /q "MemoryGranulatorMode-Windows-VST3.zip"
powershell -NoProfile -Command "Compress-Archive -Path 'dist\*' -DestinationPath 'MemoryGranulatorMode-Windows-VST3.zip' -Force"
if errorlevel 1 (
    echo ERRORE: non riesco a creare lo zip.
    pause
    exit /b 1
)

echo.
echo [8/8] Fatto.
echo.
echo ============================================
echo  Plugin: %CD%\%SRC%
echo  Pannello: %CD%\docs\panel.png
echo  Pannello 2x: %CD%\docs\panel_2x.png
echo  Zip:    %CD%\MemoryGranulatorMode-Windows-VST3.zip
echo ============================================
echo.

set "INSTALLED=%LOCALAPPDATA%\Programs\Common\VST3\MemoryGranulatorMode.vst3"
if exist "%INSTALLED%" goto :localInstall

echo  Installato in: C:\Program Files\Common Files\VST3
echo  In Ableton: Preferenze -^> Plug-in -^> Riscansiona.
goto :abletonDone

:localInstall
echo  Installato in: %INSTALLED%
echo  In Ableton: Preferenze -^> Plug-in -^> "cartella VST3 personalizzata":
echo    incolla  %LOCALAPPDATA%\Programs\Common\VST3  poi Riscansiona.

:abletonDone
echo.
echo  Per usare MIDI in Ableton, consulta la sezione "Instradamento MIDI"
echo  nel file LEGGIMI incluso nello zip.
echo.
echo  (Se il build gira come amministratore, la copia va direttamente nella
echo  cartella VST3 di sistema e questo passaggio in Ableton non serve.)
echo.
pause
