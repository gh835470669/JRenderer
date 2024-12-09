set INPUT_FILE=%~nx1
set OUTPUT_DIRECTORY=%~dp1
set OUTPUT_FILE=%INPUT_FILE:.=_%.spv
%~dp0..\..\third_party\bin\glslc.exe %1 -o %OUTPUT_DIRECTORY%%OUTPUT_FILE%
rem usage in terminal:  .\res\shaders\compile.bat .\res\shaders\star_rail.frag
rem output file: .\res\shaders\star_rail_frag.spv