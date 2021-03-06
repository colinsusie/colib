/**
 * 文件系统的一些常用接口
 */
#define LUA_LIB
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "lua.h"
#include "lauxlib.h"
#include "coconf.h"

#if defined(_WIN32)
	#include <direct.h>
	#include <windows.h>
	#include <io.h>
#else
	#include <dirent.h>
	#include <sys/types.h>
	#include <unistd.h>
#endif // defined(_WIN32)

#ifdef _WIN32
	#define SEP "\\"
	#define ALLSEPS "\\/"
	#define FUNC_STAT _stati64
	#define FUNC_LSTAT FUNC_STAT
	#define STRUCT_STAT struct _stat64 
	#define chdir(p) _chdir(p)
	#define getcwd(d, s) _getcwd(d, s)
	#define rmdir(p) _rmdir(p)
#else
	#define SEP "/"
	#define ALLSEPS "/"
	#define FUNC_STAT stat
	#define FUNC_LSTAT lstat
	#define STRUCT_STAT struct stat
#endif // _WIN32


#define DIR_ITR "CO_DIR_ITR"

typedef struct diritr {
	int closed;			// 是否已经关闭
#if defined(_WIN32)
	intptr_t handle;
	char *path;
	int first_time;
#else
	DIR *dir;
#endif
} diritr_t;


static void _diritr_close(diritr_t*itr) {
	if (itr->closed) return;
#if defined(_WIN32)
	if (itr->handle != -1L)
		_findclose(itr->handle);
	if (itr->path)
		co_free(itr->path);
#else
	if (itr->dir)
		closedir(itr->dir);
#endif
	itr->closed = 1;
}

static int l_diritr_close(lua_State *L) {
	diritr_t *itr = (diritr_t*)luaL_checkudata(L, 1, DIR_ITR);
	_diritr_close(itr);
	return 0;
}

static int _diritr_next(lua_State *L) {
	diritr_t *itr = (diritr_t*)lua_touserdata(L, lua_upvalueindex(1));
	if (itr->closed) luaL_error(L, "Scan a closed directory");
#if defined(_WIN32)
	struct _finddata_t finddata;
	while (1) {
		if (itr->first_time) {
			itr->first_time = 0;
			itr->handle = _findfirst(itr->path, &finddata);
			if (itr->handle == -1L) {
				_diritr_close(itr);
				if (errno != ENOENT) {
					lua_pushnil(L);
					lua_pushstring(L, strerror(errno));
					return 2;
				} else {
					return 0;
				}
			}
		} else {
			if (_findnext(itr->handle, &finddata) == -1L) {
				_diritr_close(itr);
				return 0;
			}
		}
		if (strcmp(finddata.name, ".") == 0 || strcmp(finddata.name, "..") == 0)
			continue;
		lua_pushstring(L, finddata.name);
		return 1;
	}
#else
	struct dirent *ent;
	while (1) {
		ent = readdir(itr->dir);
		if (ent == NULL) {
			closedir(itr->dir);
			itr->closed = 1;
			return 0;
		}
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;
		lua_pushstring(L, ent->d_name);
		return 1;
	}
#endif	
}

static int l_scandir(lua_State *L) {
	size_t pathsz;
	const char *path = luaL_checklstring(L, 1, &pathsz);
	diritr_t *itr = (diritr_t*)co_newuserdata(L, sizeof(diritr_t));
	memset(itr, 0, sizeof(*itr));
	luaL_setmetatable(L, DIR_ITR);
#if defined(_WIN32)
	itr->handle = -1L;
	itr->path = (char*)co_malloc(pathsz + 5);
	strncpy(itr->path, path, pathsz);
	char ch = itr->path[pathsz-1];
	if (!strchr(ALLSEPS, ch) && ch != ':')
		itr->path[pathsz++] = SEP[0];
	itr->path[pathsz++] = L'*';
	itr->path[pathsz] = '\0';
	itr->first_time = 1;
#else
	DIR *dir = opendir(path);
	if (dir == NULL)
		luaL_error(L, "cannot open %s: %s", path, strerror(errno));
	itr->dir = dir; 
#endif
	lua_pushcclosure(L, _diritr_next, 1);
	return 1;
}

static int l_exists(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	STRUCT_STAT st;
	if(FUNC_STAT(path, &st) == 0) 
		lua_pushboolean(L, 1);
	else 
		lua_pushboolean(L, 0);
	return 1;
}

static int l_getsize(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	STRUCT_STAT st;
	if(FUNC_STAT(path, &st) == 0) {
		lua_pushinteger(L, (lua_Integer)st.st_size);
		return 1;
	} else {
		lua_pushnil(L);
		lua_pushfstring(L, "getsize error: %s", strerror(errno));
		return 2;
	}
}

///////////////////////////////////////////////////////////////////////
// 消除一些系统的差异
#ifdef _WIN32
// 取文件时间
static int win_getfiletime(const char *path, FILETIME *ftCreate, FILETIME *ftAccess, FILETIME *ftWrite) {
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return -1;
	}
	if (!GetFileTime(hFile, ftCreate, ftAccess, ftWrite)) {
		CloseHandle(hFile);
		return -1;  
	} else {
		CloseHandle(hFile);
		return 0;
	}
}

static __int64 secs_between_epochs = 11644473600; /* Seconds between 1.1.1601 and 1.1.1970 */
// 将文件时间转换成秒和纳秒
static void win_convert_filetime(FILETIME *time_in, time_t *time_out, long* nsec_out) {
	__int64 in = (int64_t)time_in->dwHighDateTime << 32 | time_in->dwLowDateTime ;
    *nsec_out = (long)(in % 10000000) * 100; 	/* FILETIME is in units of 100 nsec. */
    *time_out = (time_t)((in / 10000000) - secs_between_epochs);
}
#elif __APPLE__
	#define st_mtim st_atimespec
	#define st_atim st_mtimespec
	#define st_ctim st_ctimespec
#else
#endif

static int l_getmtime(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	time_t sec;
	long nsec;
#ifdef _WIN32
	FILETIME ftWrite;
	if (win_getfiletime(path, NULL, NULL, &ftWrite) != 0) {
		lua_pushnil(L);
		lua_pushfstring(L, "getmtime error: %d", GetLastError());
		return 2;
	}
	win_convert_filetime(&ftWrite, &sec, &nsec);
#else
	STRUCT_STAT st;
	if(FUNC_STAT(path, &st) != 0) {
		lua_pushnil(L);
		lua_pushfstring(L, "getmtime error: %s", strerror(errno));
		return 2;
	}
	sec = st.st_mtim.tv_sec;
	nsec = st.st_mtim.tv_nsec;
#endif
	lua_pushnumber(L, (lua_Number)(sec + 1e-9*nsec));
	return 1;
}

static int l_getatime(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	time_t sec;
	long nsec;
#ifdef _WIN32
	FILETIME ftAccess;
	if (win_getfiletime(path, NULL, &ftAccess, NULL) != 0) {
		lua_pushnil(L);
		lua_pushfstring(L, "getatime error: %d", GetLastError());
		return 2;
	}
	win_convert_filetime(&ftAccess, &sec, &nsec);
#else
	STRUCT_STAT st;
	if(FUNC_STAT(path, &st) != 0) {
		lua_pushnil(L);
		lua_pushfstring(L, "getmtime error: %s", strerror(errno));
		return 2;
	}
	sec = st.st_atim.tv_sec;
	nsec = st.st_atim.tv_nsec;
#endif
	lua_pushnumber(L, (lua_Number)(sec + 1e-9*nsec));
	return 1;
}

static int l_getctime(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	time_t sec;
	long nsec;
#ifdef _WIN32
	FILETIME ftCreate;
	if (win_getfiletime(path, &ftCreate, NULL, NULL) != 0) {
		lua_pushnil(L);
		lua_pushfstring(L, "getatime error: %d", GetLastError());
		return 2;
	}
	win_convert_filetime(&ftCreate, &sec, &nsec);
#else
	STRUCT_STAT st;
	if(FUNC_STAT(path, &st) != 0) {
		lua_pushnil(L);
		lua_pushfstring(L, "getmtime error: %s", strerror(errno));
		return 2;
	}
	sec = st.st_ctim.tv_sec;
	nsec = st.st_ctim.tv_nsec;
#endif
	lua_pushnumber(L, (lua_Number)(sec + 1e-9*nsec));
	return 1;
}

static int l_getmode(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	STRUCT_STAT st;
	if(FUNC_STAT(path, &st) == 0) {
		lua_pushinteger(L, (lua_Integer)st.st_mode);
		return 1;
	} else {
		lua_pushnil(L);
		lua_pushfstring(L, "getmode error: %s", strerror(errno));
		return 2;
	}
}

static int l_getlinkmode(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	STRUCT_STAT st;
	if(FUNC_LSTAT(path, &st) == 0) {
		lua_pushinteger(L, (lua_Integer)st.st_mode);
		return 1;
	} else {
		lua_pushnil(L);
		lua_pushfstring(L, "getlinkmode error: %s", strerror(errno));
		return 2;
	}
}

static int l_mkdir(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	int err;
#ifdef _WIN32
	err = _mkdir(path);
#else
	err =  mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
#endif
	if (err) {
		lua_pushboolean(L, 0);
		lua_pushfstring(L, "mkdir error: %s", strerror(errno));
		return 2;
	} else {
		lua_pushboolean(L, 1);
		return 1;
	}
}

static int l_rmdir(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	int err = rmdir(path);
	if (err) {
		lua_pushboolean(L, 0);
		lua_pushfstring(L, "rmdir error: %s", strerror(errno));
		return 2;
	} else {
		lua_pushboolean(L, 1);
		return 1;
	}
}

static int l_chdir(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
	if (chdir(path)) {
		lua_pushnil (L);
		lua_pushfstring (L,"chdir error: %s", strerror(errno));
		return 2;
	} else {
		lua_pushboolean (L, 1);
		return 1;
	}
}

static int l_getcwd(lua_State *L) {
	char path[256];
	int size = 256;
	if (getcwd(path, size) != NULL) {
		lua_pushstring(L, path);
		return 1;
	}
	char *buff = NULL;
	int result;
	while (1) {
		size <<= 1;
		buff = (char*)co_realloc(buff, size);
		if (buff == NULL) {
			lua_pushnil(L);
			lua_pushfstring(L, "getcwd error: realloc() failed");
			result = 2;
			break;
		}
		if (getcwd(buff, size) != NULL) {
			lua_pushstring(L, buff);
			result = 1;
			break;
		}
		if (errno != ERANGE) {
			lua_pushnil(L);
			lua_pushfstring(L, "getcwd error: %s", strerror(errno));
			result = 2;
			break;
		}
	}
	co_free(buff);
	return result;
}

static const luaL_Reg dirmt[] = {
	{"__gc", l_diritr_close},
	{NULL, NULL}
};

static const luaL_Reg lib[] = {
	{"scandir", l_scandir},
	{"exists", l_exists},
	{"getsize", l_getsize},
	{"getmtime", l_getmtime},
	{"getatime", l_getatime},
	{"getctime", l_getctime},
	{"getmode", l_getmode},
	{"getlinkmode", l_getlinkmode},
	{"mkdir", l_mkdir},
	{"rmdir", l_rmdir},
	{"chdir", l_chdir},
	{"getcwd", l_getcwd},
	{NULL, NULL}
};

static void init_consts(lua_State *L) {
	lua_pushliteral(L, SEP);
	lua_setfield(L, -2, "sep");
	lua_pushliteral(L, ALLSEPS);
	lua_setfield(L, -2, "allseps");
#ifdef _WIN32
	lua_pushboolean(L, 1);
	lua_setfield(L, -2, "iswindows");
#endif
}

LUAMOD_API int luaopen_colibc_filesys(lua_State *L) {
	luaL_checkversion(L);
	luaL_newmetatable(L, DIR_ITR);
	luaL_setfuncs(L, dirmt, 0);

	luaL_newlib(L, lib);
	init_consts(L);
	return 1;
}