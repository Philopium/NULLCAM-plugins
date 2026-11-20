@echo off
setlocal
cd /d "%~dp0"

echo ============================================
echo  HarmonicResonatorMode - build VST3 per Windows
echo ============================================
echo.

where cmake >nul 2>nul
if errorlevel 1 (
    echo ERRORE: cmake non e' nel PATH.
    pause
    exit /b 1
)

where git >nul 2>nul
if errorlevel 1 (
    echo ATTENZIONE: git non trovato. JUCE viene scaricato da GitHub, quindi serve.
    echo Se gia' hai una copia di JUCE, interrompi con CTRL+C e usa:
    echo    cmake -B build -DJUCE_DIR=C:\percorso\JUCE
    pause
)

echo [1/6] Configurazione...
cmake -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo.
    echo Configurazione fallita. Cause piu' comuni:
    echo  - manca Visual Studio con il carico di lavoro "Sviluppo di applicazioni desktop con C++"
    echo  - il generatore non e' VS 2022: prova a togliere  -G "Visual Studio 17 2022" -A x64
    pause
    exit /b 1
)

echo.
echo [2/6] Compilazione (la prima volta scarica anche JUCE, ci vuole qualche minuto)...
cmake --build build --config Release --target HarmonicResonatorMode_VST3 HarmonicResonatorMode_Standalone --parallel
if errorlevel 1 (
    echo.
    echo Compilazione fallita: copia le ultime righe di errore.
    pause
    exit /b 1
)

set "SRC=build\HarmonicResonatorMode_artefacts\Release\VST3\HarmonicResonatorMode.vst3"
if not exist "%SRC%" (
    echo ERRORE: non trovo "%SRC%"
    pause
    exit /b 1
)

echo.
echo [3/6] Test del motore (non serve un DAW)...
cmake --build build --config Release --target harmonicresonator_dsp_test
build\Release\harmonicresonator_dsp_test.exe
if errorlevel 1 (
    echo.
    echo I test del DSP sono falliti: guarda il messaggio sopra.
    pause
    exit /b 1
)

echo.
echo [4/6] Render del pannello + test dell'interfaccia (non serve un DAW)...
cmake --build build --config Release --target harmonicresonator_ui_check
build\harmonicresonator_ui_check_artefacts\Release\harmonicresonator_ui_check.exe docs\panel.png 1
build\harmonicresonator_ui_check_artefacts\Release\harmonicresonator_ui_check.exe docs\panel_2x.png 2
if errorlevel 1 (
    echo.
    echo I test dell'interfaccia sono falliti: guarda il messaggio sopra.
    pause
    exit /b 1
)

echo.
echo [5/6] Prova a caricare il VST3 costruito, come fa un host...
cmake --build build --config Release --target harmonicresonator_vst3_check
build\harmonicresonator_vst3_check_artefacts\Release\harmonicresonator_vst3_check.exe
if errorlevel 1 (
    echo.
    echo Il VST3 non si carica: guarda il messaggio sopra.
    pause
    exit /b 1
)

echo.
echo [6/6] Preparo lo zip da mandare...
if not exist dist mkdir dist
if exist "dist\HarmonicResonatorMode.vst3" rmdir /s /q "dist\HarmonicResonatorMode.vst3"
xcopy /E /I /Y "%SRC%" "dist\HarmonicResonatorMode.vst3" >nul
copy /Y "INSTALLAZIONE.md" "dist\LEGGIMI - installazione.md" >nul
copy /Y "..\LICENSE.md" "dist\LICENZA - AGPLv3.md" >nul
if exist "HarmonicResonatorMode-Windows-VST3.zip" del /q "HarmonicResonatorMode-Windows-VST3.zip"
powershell -NoProfile -Command "Compress-Archive -Path 'dist\*' -DestinationPath 'HarmonicResonatorMode-Windows-VST3.zip' -Force"

echo.
echo ============================================
echo  Fatto
echo ============================================
echo  Plugin: %CD%\%SRC%
echo  Pannello: %CD%\docs\panel.png
echo  Zip:    %CD%\HarmonicResonatorMode-Windows-VST3.zip
echo.
set "INSTALLED=%LOCALAPPDATA%\Programs\Common\VST3\HarmonicResonatorMode.vst3"
if exist "%INSTALLED%" goto :localInstall

echo  Installato in: C:\Program Files\Common Files\VST3
echo  In Ableton: Impostazioni -^> Plug-in -^> Riscansiona.
goto :abletonDone

:localInstall
echo  Installato in: %INSTALLED%
echo  In Ableton: Impostazioni -^> Plug-in -^> "cartella VST3 personalizzata":
echo    incolla  %LOCALAPPDATA%\Programs\Common\VST3  poi Riscansiona.

:abletonDone
echo.
echo  (Se il build gira come amministratore, la copia va direttamente nella
echo  cartella VST3 di sistema e questo passaggio in Ableton non serve.)
echo.
pause
