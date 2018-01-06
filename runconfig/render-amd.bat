:: copy parameters
set /p pflags=<%1

:: show this model
cd ../ 
"./RelWithDebInfo/satellite-gltf.exe" %pflags% -g 0 -p amd -b noon_grass_8k.exr 
