:: copy parameters
set /p pflags=<%1

:: show this model
cd ../ 
"./Release/satellite-gltf.exe" %pflags% -g 0 -p nvidia -b rustig_koppie_8k.exr 
