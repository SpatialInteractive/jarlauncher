#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define JAVA_CMD L"java.exe"

wchar_t* get_env_var(wchar_t* name);
wchar_t* read_jre_reg_value(wchar_t* value_name, int try64, int tr32);
int valid_home(wchar_t* java_home);
void parse_command_line(wchar_t** out_this_file, wchar_t** out_command_line_tail);

int wmain(int argc, wchar_t** argv)
{
	wchar_t *java_home;
	wchar_t *java_exec;
	wchar_t *this_file;
	wchar_t *command_line_tail;
	wchar_t *new_command_line;
	PROCESS_INFORMATION pinfo;
	STARTUPINFO sinfo;
	DWORD rc;

	/* Search for java home:
		Environment variable JAVA_HOME
		64bit Java Registry
		32bit Java Registry
		Default Java registry
	*/
	java_home=get_env_var(L"JAVA_HOME");
	if (!valid_home(java_home)) {
		java_home=read_jre_reg_value(L"JavaHome", 1, 0);
		if (!valid_home(java_home)) {
			java_home=read_jre_reg_value(L"JavaHome", 0, 1);
			if (!valid_home(java_home)) {
				java_home=read_jre_reg_value(L"JavaHome", 0, 0);
				if (!valid_home(java_home)) {
					if (java_home) free(java_home);
					java_home=0;
				}
			}
		}
	}

	if (!java_home) {
		/* No home directory found - just leave it up to the path */
		java_exec=(wchar_t*) malloc(sizeof(wchar_t) * (wcslen(JAVA_CMD)+1));
		wcscpy(java_exec, JAVA_CMD);
	} else {
		java_exec=(wchar_t*) malloc(sizeof(wchar_t) * (wcslen(java_home) + wcslen(JAVA_CMD) + 64));
		wcscpy(java_exec, java_home);
		wcscat(java_exec, L"\\bin\\");
		wcscat(java_exec, JAVA_CMD);
	}
	
	/* Break out command line parts */
	parse_command_line(&this_file, &command_line_tail);

	new_command_line=(wchar_t*) malloc(sizeof(wchar_t) * 
		(2*wcslen(this_file) + wcslen(command_line_tail) + wcslen(java_exec) + 64));
	wsprintf(new_command_line, L"\"%s\" \"-Dexec.file=%s\" -jar \"%s\" %s",
		java_exec, this_file, this_file, command_line_tail);

	/*wprintf(L"New command line: %s\n", new_command_line);*/
	
	memset(&sinfo, 0, sizeof(sinfo));
	if (!CreateProcess(
			NULL, 
			new_command_line, 
			NULL, 
			NULL, 
			TRUE,
			0, 
			NULL, 
			NULL, 
			&sinfo, 
			&pinfo))
	{
		fwprintf(stderr, L"ERROR Launching sub-process\n");
		return 1;
	}
	
	WaitForSingleObject(pinfo.hProcess, INFINITE);
	GetExitCodeProcess(pinfo.hProcess, &rc);

	return rc;
}

wchar_t* get_env_var(wchar_t* name)
{
	DWORD rc;
	wchar_t *buffer;
	DWORD size=512;

	buffer=(wchar_t*) malloc(sizeof(wchar_t) * size);
	for (;;) {
		rc=GetEnvironmentVariable(name, buffer, size);
		if (rc==0) {
			free(buffer);
			return 0;
		}
		if (rc<size) return buffer;
		else {
			size=rc;
			buffer=(wchar_t*) realloc(buffer, size);
			if (!buffer) return 0;
		}
	}
}

wchar_t* read_jre_reg_value(wchar_t* value_name, int try64, int try32)
{
	LONG rc;
	size_t buffer_size;
	wchar_t *buffer;
	DWORD cb, type;
	HKEY hk_jre=0, hk_ver=0;
	REGSAM sam_desired=KEY_READ;

	/* Add flags for explicit 32 or 64 bit selection */
	if (try64) sam_desired|=KEY_WOW64_64KEY;
	else if (try32) sam_desired|=KEY_WOW64_32KEY;

	/* Open the top level key */
	rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\Java Runtime Environment", 
		0, sam_desired, &hk_jre);
	if (rc!=ERROR_SUCCESS) return 0;

	/* Get the CurrentVersion value */
	buffer_size=512;
	buffer=(wchar_t*) malloc(buffer_size);
	for (;;) {
		cb=buffer_size-sizeof(wchar_t);
		rc=RegQueryValueEx(hk_jre, L"CurrentVersion", 0, &type, (LPBYTE)buffer, &cb);
		if (rc==ERROR_MORE_DATA) {
			buffer_size=cb+sizeof(wchar_t);
			buffer=(wchar_t*)realloc(buffer, buffer_size);
			if (!buffer) return 0;
			continue;
		} else {
			break;
		}
	}

	if (rc!=ERROR_SUCCESS || type!=REG_SZ) {
		RegCloseKey(hk_jre);
		free(buffer);
		return 0;
	}
	buffer[cb]=0;

	/* Open the version specific key */
	rc=RegOpenKeyEx(hk_jre, buffer, 0, sam_desired, &hk_ver);
	if (rc!=ERROR_SUCCESS) {
		RegCloseKey(hk_jre);
		free(buffer);
		return 0;
	}

	for (;;) {
		cb=buffer_size-sizeof(wchar_t);
		rc=RegQueryValueEx(hk_ver, value_name, 0, &type, (LPBYTE)buffer, &cb);
		if (rc==ERROR_MORE_DATA) {
			buffer_size=cb+sizeof(wchar_t);
			buffer=(wchar_t*)realloc(buffer, buffer_size);
			if (!buffer) return 0;
			continue;
		} else {
			break;
		}
	}
	
	RegCloseKey(hk_ver);
	RegCloseKey(hk_jre);
	if (rc==ERROR_SUCCESS && type==REG_SZ) {
		buffer[cb]=0;
		return buffer;
	} else {
		free(buffer);
		return 0;
	}
}

int valid_home(wchar_t* java_home)
{
	DWORD attr;
	wchar_t* check_file;
	if (!java_home) return 0;
	
	/* probe for a valid java.exe */
	check_file=(wchar_t*) malloc(sizeof(wchar_t) * (wcslen(java_home) + 64));
	wcscpy(check_file, java_home);
	wcscat(check_file, L"\\bin\\java.exe");
	
	attr=GetFileAttributes(check_file);
	if (attr==INVALID_FILE_ATTRIBUTES || (attr&FILE_ATTRIBUTE_DIRECTORY)) return 0;
	return 1;
}

void parse_command_line(wchar_t** out_this_file, wchar_t** out_command_line_tail)
{
	wchar_t* cmd_line=_wcsdup(GetCommandLine());
	wchar_t* break_line;
	wchar_t break_char;

	/* Find the break point for the first segment.  If starts with a quote,
	   then this is the position of the next quote.  Otherwise, it is the next
	   space.
	*/
	if (cmd_line[0]=='"') {
		break_char='"';
		*out_this_file=break_line=cmd_line+1;
	} else if (cmd_line[0]=='\'') {
		break_char='\'';
		*out_this_file=break_line=cmd_line+1;
	} else {
		break_char=' ';
		*out_this_file=break_line=cmd_line;
	}

	while (*break_line && *break_line!=break_char) break_line++;
	if (*break_line) {
		*break_line=0;
		*out_command_line_tail=break_line+1;
	} else {
		*out_command_line_tail=break_line;
	}

	/* dup them and free */
	*out_this_file=_wcsdup(*out_this_file);
	*out_command_line_tail=_wcsdup(*out_command_line_tail);
}
