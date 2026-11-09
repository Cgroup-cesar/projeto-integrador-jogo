@echo off
rem Baixa a fonte VT323 (licenca SIL OFL 1.1) para assets\fontes\. E opcional: sem ela o jogo usa a fonte padrao.
cd /d "%~dp0\.."
if not exist assets\fontes mkdir assets\fontes
set BASE=https://github.com/google/fonts/raw/main/ofl/vt323
curl -fL -o assets\fontes\VT323-Regular.ttf %BASE%/VT323-Regular.ttf || goto erro
curl -fL -o assets\fontes\OFL-VT323.txt %BASE%/OFL.txt || goto erro
echo Fonte salva em assets\fontes\VT323-Regular.ttf
exit /b 0
:erro
echo Nao foi possivel baixar a fonte. O jogo funciona sem ela (usa a fonte padrao).
exit /b 1
