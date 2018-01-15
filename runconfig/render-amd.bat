:: copy parameters
set /p pflags=<%1

:: show this model
cd ../ 
"./RelWithDebInfo/satellite-gltf.exe" %pflags% -g 0 -p amd -b kiara_4_mid-morning_8k.exr 
