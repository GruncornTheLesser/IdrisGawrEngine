echo Compiling Shaders...



for %%a in (Resources\*.frag) do (
	echo %%a
	C:/src/VulkanSDK/1.3.224.1/Bin/glslc.exe %%a -o %%a.spv
)

for %%a in (Resources\*.vert) do (
	echo %%a
	C:/src/VulkanSDK/1.3.224.1/Bin/glslc.exe %%a -o %%a.spv
)

for %%a in (Resources\*.geom) do (
	echo %%a
	C:/src/VulkanSDK/1.3.224.1/Bin/glslc.exe %%a -o %%a.spv
)

echo Compiling Program...