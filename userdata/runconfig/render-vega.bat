:: copy parameters
set /p pflags=<%1

:: show this model
cd ../ 
start /high /b /min ./RelWithDebInfo/satellite-gltf.exe %pflags% -g 1 -p shaders/amd -b models/kiara_4_mid-morning_8k.exr 
