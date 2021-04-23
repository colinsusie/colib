---@type cofilesys
local cofilesys = require "colib.filesys"
local costr = require "colib.str"
local strsub = string.sub
local strrep = string.rep
local strgsub = string.gsub
local strbyte = string.byte
local ipairs = ipairs
local tconcat = table.concat

local S_IFMT   = 0xF000		---File type mask

-- 各种文件类型
cofilesys.S_IFIFO  = 0x1000		---FIFO
cofilesys.S_IFCHR  = 0x2000		---Character device
cofilesys.S_IFDIR  = 0x4000		---Directory
cofilesys.S_IFBLK  = 0x6000		---Block device
cofilesys.S_IFREG  = 0x8000		---Regular file
cofilesys.S_IFLNK  = 0xA000   	---Symbolic link
cofilesys.S_IFSOCK = 0xC000   	---Socket

---分隔符
local sep = cofilesys.sep
local allseps = cofilesys.allseps
local iswindows = cofilesys.iswindows


---返回目录中的文件列表，不会包含 . 和 .. 两个特殊目录
---@param path string 目录路径
function cofilesys.listdir(path)
	local ls = {}
	local i = 1
	for name in cofilesys.scandir(path) do
		ls[i] = name
		i = i + 1
	end
	return ls
end

---检查路径是否是目录
function cofilesys.isdir(path)
	local mode = cofilesys.getmode(path)
	if mode then return (mode & S_IFMT) == cofilesys.S_IFDIR end
end

---检查路径是否是普通文件
function cofilesys.isfile(path)
	local mode = cofilesys.getmode(path)
	if mode then return (mode & S_IFMT) == cofilesys.S_IFREG end
end

---取文件的类型: cofilesys.S_IFIFO...
function cofilesys.filetype(path)
	local mode = cofilesys.getmode(path)
	return mode & S_IFMT
end

---分隔路径的扩展名，返回root和ext两部分，root + ext = path
---如 abc/aaa.txt => abc/aaa  .txt
---@param path string 路径
---@return string, string 返回root和ext两部分
function cofilesys.splitext(path)
	local doti = costr.rfind(path, ".")
	if doti ~= 0 then
		return strsub(path, 1, doti-1), strsub(path, doti)
	end
	return path, ""
end

---修改路径的扩展名，如chgext("aaa.txt", ".jpg") => aaa.jpg
---@param path string 路径
---@param ext string 扩展名，比如 .jpg
---@return string 返回新的路径
function cofilesys.chgext(path, ext)
	local root, _ = cofilesys.splitext(path)
	return root .. ext
end

------------------------------------------------------------------------------------------
-- windows: 规则太多太Ugly，暂时不想处理

local function _win_splitdrive(path)
	if #path > 2 then
		local normp = strgsub(path, "/", "\\")
		local c1, c2, c3 = strbyte(normp, 1, 3)
		-- UNC \\machine\mountpoint\directory\etc\...
		if c1 == 92 and c2 == 92 and c3 ~= 92 then	-- 92 = '\'
			local index = normp:find("\\", 3, true)
			if not index then
				return "", path
			end
			local index2 = normp:find("\\", index+1, true) or #path
			if index2 == index +1 then
				return "", path
			end
			return strsub(path, 1, index2), strsub(path, index2+1)
		end
		if strbyte(normp, 2) == 58 then	-- 58 == ':'
			return strsub(path, 1, 2), strsub(path, 3)
		end
	end
	return "", path
end

local function _win_split(path)
	local d, p = _win_splitdrive(path)
	print(d, p)
	local i = costr.rfind(p, allseps, true) + 1
	local head, tail = strsub(p, 1, i-1), strsub(p, i)
	local nhead = costr.rstrip(head, allseps)
	if nhead == "" then
		nhead = head
	end
	return d .. head, tail
end

local function _win_join(path, ...)
	-- TODO
end

local function _win_isabs(path)
	local _, dir = _win_splitdrive(path)
	return dir:find("^[\\/]") ~= nil
end

local function _win_abspath(path)
	-- TODO
end

local function _win_normpath(path)
	-- TODO
end

------------------------------------------------------------------------------------------
-- posix

local function _posix_split(path)
	local i = costr.rfind(path, sep) + 1
	local head, tail = strsub(path, 1, i-1), strsub(path, i)
	if head ~= "" and head ~= strrep(sep, #head) then
		head = costr.rstrip(head, sep)
	end
	return head, tail
end

local function _posix_join(path, ...)
	local t = {...}
	for i, b in ipairs(t) do
		if costr.startswith(b, "/") then
			path = b
		elseif path == '' or costr.endswith(path, "/") then
			path = path .. b
		else
			path = path .. "/" .. b
		end
	end
	return path
end

local function _posix_isabs(path)
	return costr.startswith(path, "/")
end

local function _posix_normpath(path)
	if path == "" then
		return "."
	end
	local init_slashes = costr.startswith(path, "/")
	if init_slashes then
		-- POSIX allows one or two initial slashes, but treats three or more as single slash.
		if costr.startswith(path, "//") and not costr.startswith(path, "///") then
			init_slashes = 2
		else
			init_slashes = 1
		end
	end
	local comps = costr.split(path, "/")
	local newcomps = {}
	local n
	for _, comp in ipairs(comps) do
		n = #newcomps
		if comp ~= "" and comp ~= "." then
			if (comp ~= "..") or (not init_slashes and n == 0) or (n > 0 and newcomps[n] == "..") then
				newcomps[n+1] = comp
			elseif n > 0 then
				newcomps[n] = nil
			end
		end
	end
	path = tconcat(newcomps, "/")
	if init_slashes then
		path = strrep("/", init_slashes) .. path
	end
	return path == "" and "." or path
end

local function _posix_abspath(path)
	if not _posix_isabs(path) then
		local cwd = cofilesys.getcwd()
		path = _posix_join(cwd, path)
	end
	return _posix_normpath(path)
end

local function _posix_splitdrive(path)
	return "", path
end


if iswindows then
	---分隔路径为 (dir, basename) 两部分： dir为目录部分，basename为文件名部分
	---dir 除了根目录之外，尾部的分隔符会被剔除。
	---basename 开头的分隔符会被剔除
	---比如： aa/bb.txt  =>  aa  bb.txt
	---      aa/bb//cc.txt  =>  aa/bb  cc.txt
	---      /aa/bb.txt  =>  /aa  bb.txt
	---@param path string 路径
	---@return string,string
	cofilesys.split = _win_split
	---连接路径，如 join("aaa", "bbb", "ccc.txt") => aaa/bbb/ccc.txt
	---@param path string 路径
	---@vararg string[] 连接的各个路径部分
	cofilesys.join = _win_join
	---判断路径是否是绝对路径
	---@param path string 路径
	cofilesys.isabs = _win_isabs
	---返回绝对路径，相对于当前工作目录的
	---@param path string 路径
	cofilesys.abspath = _win_abspath
	---规范化路径，如 A//B, A/./B 和 A/foo/../B 全部变成 A/B.
	------@param path string 路径
	cofilesys.normpath = _win_normpath
	---分隔盘符和路径，只有Windows有用，其他系统盘符为空
	---如： C:\aa\bb => C:  \aa\bb
	cofilesys.splitdrive = _win_splitdrive
else
	cofilesys.split = _posix_split
	cofilesys.join = _posix_join
	cofilesys.isabs = _posix_isabs
	cofilesys.abspath = _posix_abspath
	cofilesys.normpath = _posix_normpath
	cofilesys.splitdrive = _posix_splitdrive

	-- TODO:
	-- walk
	-- mkdirs
	-- rmtree
	-- ...
end

return cofilesys