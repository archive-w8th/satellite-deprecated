:: copy parameters
set /p pflags=<%1

:: show this model
cd ../ 
start /high /b /min ./RelWithDebInfo/satellite-gltf.exe %pflags% -g 0 -p intel -b kiara_4_mid-morning_8k.exr
