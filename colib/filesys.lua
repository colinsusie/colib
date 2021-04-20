---@type cofilesys
local cofilesys = require "colib.filesys"

local S_IFMT   = 0xF000		---File type mask

-- 各种文件类型
cofilesys.S_IFIFO  = 0x1000		---FIFO
cofilesys.S_IFCHR  = 0x2000		---Character device
cofilesys.S_IFDIR  = 0x4000		---Directory
cofilesys.S_IFBLK  = 0x6000		---Block device
cofilesys.S_IFREG  = 0x8000		---Regular file
cofilesys.S_IFLNK  = 0xA000   	---Symbolic link
cofilesys.S_IFSOCK = 0xC000   	---Socket


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

return cofilesys