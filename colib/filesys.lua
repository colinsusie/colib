---@type cofilesys
local cofilesys = require "colib.filesys"
local costr = require "colib.str"
local strsub = string.sub
local strrep = string.rep

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

local function _win_split(path)
	error("comming soon")
end

local function _win_join(path, ...)
	error("comming soon")
end

local function _win_isabs(path)
	error("comming soon")
end

local function _win_abspath(path)
	error("comming soon")
end

local function _win_normpath(path)
	error("comming soon")
end


local function _posix_split(path)
	local i = costr.rfind(path, sep) + 1
	local head, tail = strsub(path, 1, i-1), strsub(path, i)
	if head ~= "" and head ~= strrep(sep, #head) then
		head = costr.rstrip(head, sep)
	end
	return head, tail
end

local function _posix_join(path, ...)
	error("comming soon")
end

local function _posix_isabs(path)
	error("comming soon")
end

local function _posix_abspath(path)
	error("comming soon")
end

local function _posix_normpath(path)
	error("comming soon")
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

	cofilesys.join = _win_join
	cofilesys.isabs = _win_isabs
	cofilesys.abspath = _win_abspath
	cofilesys.normpath = _win_normpath
else
	cofilesys.split = _posix_split
	cofilesys.join = _posix_join
	cofilesys.isabs = _posix_isabs
	cofilesys.abspath = _posix_abspath
	cofilesys.normpath = _posix_normpath

	-- TODO:
	-- walk
	-- mkdirs
	-- rmtree
	-- ...
end

return cofilesys