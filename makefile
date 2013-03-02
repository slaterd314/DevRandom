
CMD=/Build

!ifdef CLEAN
CMD=/Clean
!endif

all : x64 Win32

Debug.X64 :
	start devenv DevRandom.sln $(CMD) "Debug|x64"


Release.X64 :
	start devenv DevRandom.sln $(CMD) "Release|x64"

x64: Debug.X64 Release.X64



Debug.Win32 :
	start devenv DevRandom.sln $(CMD) "Debug|Win32"


Release.Win32 :
	start devenv DevRandom.sln $(CMD) "Release|Win32"

Win32: Debug.Win32 Release.Win32


edit :
	devenv DevRandom.sln



