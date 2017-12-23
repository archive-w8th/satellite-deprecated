:: copy parameters
set /p pflags=<%1

:: show this model
cd ../ 
"./Release/satellite-gltf.exe" %pflags% -g 1 -p amd -b rustig_koppie_8k.exr 
