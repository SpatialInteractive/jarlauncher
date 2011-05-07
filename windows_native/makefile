all: winlauncher.exe

winlauncher.exe: winlauncher.c
	cl /Os /Fewinlauncher.exe /TC /MT /DUNICODE winlauncher.c /link advapi32.lib user32.lib
	
