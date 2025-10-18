@echo off
setlocal enabledelayedexpansion

setlocal

rem --- Vérifie si pymcuprog est déjà installé ---
pymcuprog --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [INFO] pymcuprog non detecte. Tentative d'installation...

    rem --- Vérifie que Python est disponible ---
    where python >nul 2>&1
    if %errorlevel% neq 0 (
        echo [ERREUR] Python n'est pas installe ou non accessible.
        echo Installez Python depuis https://www.python.org/downloads/
        pause
        exit /b 1
    )

    rem --- Vérifie que pip est disponible ---
    python -m pip --version >nul 2>&1
    if %errorlevel% neq 0 (
        echo [ERREUR] pip n'est pas installe.
        echo Essayez : python -m ensurepip
        pause
        exit /b 1
    )

    rem --- Installe pymcuprog ---
    python -m pip install --upgrade pip >nul
    python -m pip install pymcuprog

    rem --- Vérifie si installation OK ---
    pymcuprog --version >nul 2>&1
    if %errorlevel% neq 0 (
        echo [ERREUR] L'installation de pymcuprog a echoue.
        pause
        exit /b 1
    )

    echo [OK] pymcuprog installe avec succes.
)


echo Liste des ports COM disponibles :
:: reg query HKLM\HARDWARE\DEVICEMAP\SERIALCOMM
:: set /p usedcom=Téléversement via port COM


:: Appelle PowerShell pour lister tous les noms contenant (COMx) et extraire COMx
for /f "delims=" %%A in ('powershell -Command "Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match '\(COM[0-9]+\)' } | ForEach-Object { if ($_ -match '\(COM[0-9]+\)') { $matches[0].Trim('()') } }"') do (
    echo !port_index!. %%A
)

:: Demander à l'utilisateur de choisir un port
echo.
set /p usedcom=Entrez le numero du port choisi : COM

:: Lister les fichiers .hex
echo.
echo Liste des fichiers HEX disponibles :
set /a index=0
for %%f in (*.hex) do (
    set /a index+=1
    set "file[!index!]=%%f"
    echo   !index!. %%f
)

:: Demander à l'utilisateur de choisir
echo.
set /p choice=Entrez le numero du fichier qu'il faut flasher :

:: Vérifier que le choix est valide
if not defined file[%choice%] (
    echo Choix invalide.
    pause
    exit /b
)

:: Récupérer le nom du fichier choisi
set HEX=!file[%choice%]!

:: Affichage
echo.
echo --> Flash de "!HEX!" sur le ATtiny3216 via COM%usedcom%
echo.

pymcuprog write -t uart -u com%usedcom% -d attiny3216 -f %HEX%
pymcuprog reset -t uart -u com%usedcom% -d attiny3216
pause
