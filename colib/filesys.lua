---@type cofilesys
local cofilesys = require "colib.filesys"
local costr = require "colib.str"
local strsub = string.sub
local strrep = string.rep
local strgsub = string.gsub
local strbyte = string.byte
local strformat = string.format
local strfind = string.find
local ipairs = ipairs
local tconcat = table.concat

local S_IFMT   = 0xF000			---File type mask

-- 各种文件类型
cofilesys.S_IFIFO  = 0x1000		---FIFO
cofilesys.S_IFCHR  = 0x2000		---Character device
cofilesys.S_IFDIR  = 0x4000		---Directory
cofilesys.S_IFBLK  = 0x6000		---Block device
cofilesys.S_IFREG  = 0x8000		---Regular file
cofilesys.S_IFLNK  = 0xA000		---Symbolic link
cofilesys.S_IFSOCK = 0xC000		---Socket


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

local function _do_getfiles(root, relpath, searchsubdir, filterfunc, flist)
	local ftype, fullpath, filepath
	for filename in cofilesys.scandir(root .. relpath) do
		filepath = relpath .. filename
		fullpath = root .. filepath
		ftype = cofilesys.filetype(fullpath)
		if ftype == cofilesys.S_IFDIR then
			if searchsubdir then
				_do_getfiles(root, relpath .. filename .. sep, searchsubdir, filterfunc, flist)
			end
		elseif ftype == cofilesys.S_IFREG then
			if filterfunc then
				filepath = filterfunc(root, filepath)
				if filepath then
					flist[#flist+1] = filepath
				end
			else
				flist[#flist+1] = filepath
			end
		end
	end
end

---返回root目录下的普通文件列表，不包含其他类型的文件，不处理符号链接
---@param path string 根目录
---@param searchsubdir boolean 是否搜索子目录
---@param fiterfunc fun(root: string, filepath: string): string 过滤函数
---		root 是getfiles传入的根目录
---		filepath 是相对于root的文件路径
---		如果返回nil则过滤掉，否则返回的文件名将加入列表
---@return string[] 返回文件路径列表，默认文件路径是相对于root的路径
function cofilesys.getfiles(root, searchsubdir, filterfunc)
	root = cofilesys.addslash(root)
	local flist = {}
	_do_getfiles(root, "", searchsubdir, filterfunc, flist)
	return flist
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

---检查路径是否是一个链接
function cofilesys.islink(path)
	local mode = cofilesys.getlinkmode(path)
	if mode then return (mode & S_IFMT) == cofilesys.S_IFLNK end
end

---取文件的类型: cofilesys.S_IFIFO...
function cofilesys.filetype(path)
	local mode = cofilesys.getmode(path)
	if mode then return mode & S_IFMT else return 0 end
end

---分隔路径的扩展名，返回root和ext两部分，root + ext = path
---如 abc/aaa.txt => abc/aaa  .txt
---@param path string 路径
---@return string, string 返回root和ext两部分
function cofilesys.splitext(path)
	local doti = costr.rfind(path, ".")
	if doti then
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

---给路径最后加上斜杆，Windows加\, 其他加/
---@param path string 路径
function cofilesys.addslash(path)
	local last = strsub(path, -1)
	if not strfind(allseps, last, 1, true) then
		path = path .. sep
	end
	return path
end

---删除路径最后的斜杆
---@param path string 路径
function cofilesys.delslash(path)
	return costr.rstrip(path, allseps)
end

------------------------------------------------------------------------------------------
-- windows: 规则太多太ugly

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
	local i = (costr.rfind(p, allseps, true) or 0) + 1
	local head, tail = strsub(p, 1, i-1), strsub(p, i)
	local nhead = costr.rstrip(head, allseps)
	if nhead == "" then
		nhead = head
	end
	return d .. nhead, tail
end

local function _win_normpath(path)
	if costr.startswith(path, "\\\\.\\") or costr.startswith(path, "\\\\?\\") then
		return path
	end
	path = strgsub(path, "/", "\\")
	local d, p = _win_splitdrive(path)
	if costr.startswith(p, "\\") then
		d = d .. "\\"
		p = costr.lstrip(p, "\\")
	end

	local comps = costr.split(p, "\\")
	local newcomps = {}
	local n
	for _, comp in ipairs(comps) do
		n = #newcomps
		if comp ~= "" and comp ~= "." then
			if (comp ~= "..") or (n > 0 and newcomps[n] == "..") or 
				(n == 0 and not costr.endswith(d, "\\")) then
				newcomps[n+1] = comp
			elseif n > 0 and newcomps[n] ~= ".." then
				newcomps[n] = nil
			end
		end
	end
	if d == "" and #newcomps == 0 then
		return "."
	else
		return d .. tconcat(newcomps, "\\")
	end
end

local function _win_join(path, ...)
	local t = {...}
	local rdrive, rpath = _win_splitdrive(path)
	for _, p in ipairs(t) do
		if rpath == "" or costr.endswith(rpath, "\\") or costr.endswith(rpath, "/") then
			rpath = rpath .. p
		else
			rpath = rpath .. "\\" .. p
		end
	end
	return rdrive .. rpath
end

local function _win_isabs(path)
	local _, dir = _win_splitdrive(path)
	return dir:find("^[\\/]") ~= nil
end

local function _win_abspath(path)
	if not _win_isabs(path) then
		local cwd = cofilesys.getcwd()
		path = _win_join(cwd, path)
	end
	return _win_normpath(path)
end

------------------------------------------------------------------------------------------
-- posix

local function _posix_split(path)
	local i = (costr.rfind(path, sep) or 0) + 1
	local head, tail = strsub(path, 1, i-1), strsub(path, i)
	if head ~= "" and head ~= strrep(sep, #head) then
		head = costr.rstrip(head, sep)
	end
	return head, tail
end

local function _posix_join(path, ...)
	local t = {...}
	for _, p in ipairs(t) do
		if path == '' or costr.endswith(path, "/") then
			path = path .. p
		else
			path = path .. "/" .. p
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
end

---创建路径中所有不存在的目录
---@param path string 路径
---@return boolean
function cofilesys.mkdirs(path)
	path = cofilesys.normpath(path)
	if path == "." then
		return true
	end
	local p = path
	local paths = {}
	while p ~= "" and not cofilesys.exists(p) do
		paths[#paths+1] = p
		p, _ = cofilesys.split(p)
	end
	if p ~= "" and not cofilesys.isdir(p) then
		return false, strformat("%s is not directory", p)
	end
	local ok, err
	for i = #paths, 1, -1 do
		p = paths[i]
		ok, err = cofilesys.mkdir(p)
		if not ok then
			return ok, err
		end
	end
	return true
end

return cofilesys