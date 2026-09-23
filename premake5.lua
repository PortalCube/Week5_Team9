workspace "MyEngine"
    architecture "x86_64"
    configurations { "Debug", "Release", "ObjViewer"  }
    platforms { "x86", "x64" }
    startproject "MyEngine"
    system "windows"
    systemversion "latest"
    location "."

    filter "platforms:x86"
        architecture "x86"

    filter "platforms:x64"
        architecture "x86_64"

    filter "action:vs2026"
        toolset "msc-v145"

    filter {}

externalproject "DirectXTK_Desktop_2026"
    location "Source/ThirdParty/DirectXTK"
    uuid "E0B52AE7-E160-4D32-BF3F-910B785E5A8E"
    kind "StaticLib"
    language "C++"
    configmap {
        ["ObjViewer"] = "Release"
    }

project "MyEngine"
    uuid "05383B45-2B78-451C-9197-8B61474A12BC"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    characterset "Unicode"
    staticruntime "Off"

    files {
	"**.h",
	"**.cpp",
        "Source/**.h",
        "Source/**.hpp",
        "Source/**.cpp",
        "Shader/**.hlsl",
        "Shader/**.hlsli"
    }

    removefiles {
        "Source/ThirdParty/DirectXTK/**"
    }

    includedirs {
        ".",
        "Source",
        "Source/ThirdParty/DirectXTK",
        "Source/ThirdParty/DirectXTK/Inc",
        "Source/ThirdParty/DirectXTK/Src"
    }
    
    defines { "NOMINMAX", "_CONSOLE" }
    
    -- 동적 링크는 여기에 추가
    links {
        "DirectXTK_Desktop_2026",
        "user32",
        "d3d11",
        "dxgi"
    }

    -- 미리 컴파일된 헤더로 컴파일 시간 최적화
    pchheader "pch.h"
    pchsource "Source/pch.cpp"

    -- 모든 cpp 파일에 #include "pch.h" 삽입하여 굳이 작성 안해도 되게함
    forceincludes { "pch.h" }

    warnings "Default"
    multiprocessorcompile "On"
    buildoptions { "/utf-8", "/FS" }
    linkoptions { "/DEBUG" }
	
	-- 프리 빌드, 포스트 빌드 스크립트
	prebuildmessage "빌드 전처리 단계를 실행합니다..."
	prebuildcommands {
		'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}Scripts/PreBuild.ps1"'
	}
	
	postbuildmessage "빌드 후처리 단계를 실행합니다..."
    postbuildcommands {
		'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}Scripts/PostBuild.ps1" -TargetDirectory "%{cfg.targetdir}"'
    }

    filter "configurations:Debug"
        defines { "_DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        --optimize "Full"
        symbols "Off"
        --linktimeoptimization "On"
		
		-- Release 빌드에서도 컴파일러/링커 최적화를 사용하지 않음
		-- 최적화된 바이너리가 일부 안티바이러스에서 오진되는 문제를 피하기 위함
		optimize "Off"
		functionlevellinking "Off"
		intrinsics "Off"
		stringpooling "Off"
		linktimeoptimization "Off"

   filter "configurations:ObjViewer"
        defines { "_OBJVIEWER", "NDEBUG" }
        --symbols "On"
		symbols "Off"
		
		-- Release 빌드에서도 컴파일러/링커 최적화를 사용하지 않음
		-- 최적화된 바이너리가 일부 안티바이러스에서 오진되는 문제를 피하기 위함
		optimize "Off"
		functionlevellinking "Off"
		intrinsics "Off"
		stringpooling "Off"
		linktimeoptimization "Off"

    filter "platforms:x86"
        defines { "WIN32" }
        targetdir "Binaries/Win32/%{cfg.buildcfg}"
        objdir "Intermediate/%{prj.name}/Win32/%{cfg.buildcfg}"

    filter "platforms:x64"
        targetdir "Binaries/x64/%{cfg.buildcfg}"
        objdir "Intermediate/%{prj.name}/x64/%{cfg.buildcfg}"

    filter "files:Source/ThirdParty/Imgui/**.cpp"
        warnings "Off"
        enablepch "Off"
        removeforceincludes { "pch.h" }

    filter "files:**VS.hlsl"
        shadertype "Vertex"
        shadermodel "5.0"
        shaderentry "MainVS"
        shaderobjectfileoutput "%{wks.location}/Content/Shader/%{file.basename}.cso"

    filter "files:**PS.hlsl"
        shadertype "Pixel"
        shadermodel "5.0"
        shaderentry "MainPS"
        shaderobjectfileoutput "%{wks.location}/Content/Shader/%{file.basename}.cso"

    filter "files:**.hlsli"
        buildaction "None"

    filter {}
