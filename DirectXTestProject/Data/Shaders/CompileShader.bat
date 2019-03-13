echo off

rem debug:

mkdir "asm\debug"
mkdir "bin\debug"

mkdir "asm\release"
mkdir "bin\release"

cls

echo on

rem /Od Disables optimizations (useful for debugging)
rem /Zi Enables debug information
rem /T <string> Shader type and target version
rem /E <string> shader entry point
rem /Fo <string> Compiled shader object bytecode.
rem /Rc <string> Outputs an assembly file listing (useful for debugging, checking instruction count, seeing what kind of code is being generated)

echo off

echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
echo ---------------------------------------------------------------------------------------------------------------
echo \/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/
fxc_64.exe "src/shader.fx" /Od /Zi /T vs_5_0 /E "VS" /Fo "bin\debug\shader_debug_vs.cso" /Fc "asm\debug\shader_vs.asm"

echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
echo ---------------------------------------------------------------------------------------------------------------
echo \/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/

fxc_64.exe "src/shader.fx" /Od /Zi /T ps_5_0 /E "PS" /Fo "bin\debug\shader_debug_ps.cso" /Fc "asm\debug\shader_ps.asm"

echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
echo ---------------------------------------------------------------------------------------------------------------
echo \/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/

rem release:

fxc_64.exe "src/shader.fx" /T vs_5_0 /E "VS" /Fo "bin\release\shader_release_vs.cso" /Fc "asm\release\shader_vs.asm"

echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
echo ---------------------------------------------------------------------------------------------------------------
echo \/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/\/\/\/\/\\/\/\/\/

fxc_64.exe "src/shader.fx" /T ps_5_0 /E "PS" /Fo "bin\release\shader_release_ps.cso" /Fc "asm\release\shader_ps.asm"

pause