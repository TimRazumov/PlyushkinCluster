// Hello filesystem class implementation

#include "helloFS.h"

#include <iostream>
#include <vector>
#include <string>

// include in one .cpp file
#include "Fuse-impl.h"

rpc::client client("127.0.0.1", 2280);


int HelloFS::getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
{
    auto tmp = client.call("getattr", path).as<std::vector<int>>();
	memset(stbuf, 0, sizeof(struct stat));
	if (tmp[0] == -1) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		stbuf->st_size = tmp[0];
	}

	return 0;
}

int HelloFS::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
	if (!client.call("isDir", path).as<bool>())
		return -ENOENT;

    auto tmp = client.call("readdir", path).as<std::vector<std::string>>();
	filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
    for (int i = 0; i < tmp.size(); i++) {
    	filler(buf, tmp[i].c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
    }
	return 0;
}


int HelloFS::open(const char *path, struct fuse_file_info *fi)
{
	if (!(client.call("isFile", path).as<bool>())) {
		return -ENOENT;
    }
	//if ((fi->flags & 3) != O_RDONLY)
	//	return -EACCES;

	return 0;
}


int HelloFS::read(const char *path, char *buf, size_t size, off_t offset,
		              struct fuse_file_info *)
{
	if (!(client.call("isFile").as<bool>()))
		return -ENOENT;

	auto data = client.call("read", path, size, offset).as<std::string>();
	size_t len = data.size();
	/*if ((size_t)offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str.c_str() + offset, size);
	} else
		size = 0;*/
    memcpy(buf, data.c_str(), len);

	return len;
}

int HelloFS::write(const char *path, const char *buf, size_t size,
                   off_t offset, struct fuse_file_info *) {
    int len = client.call("write", path,
                          buf, size, offset).as<int>();
    return len;
}

int HelloFS::mknod(const char *path, mode_t mode, dev_t) {
    bool ok = client.call("mknod", path).as<bool>();
    if (!ok) {
        return -errno;
    }
    return 0;
}

int HelloFS::unlink(const char *path) {
    bool ok = client.call("delete_file", path).as<bool>();
    if (!ok) {
        return -errno;
    }
    return 0;
}

int HelloFS::rename(const char* from, const char* to, unsigned int) {
    bool ok = client.call("rename", from, to).as<bool>();
    if (!ok) {
        return -errno;
    }
    return 0;
}

int HelloFS::mkdir(const char *path, mode_t) {
    bool ok = client.call("mkdir", path).as<bool>();
    if (!ok) {
        return -errno;
    }
    return 0;
}

int HelloFS::rmdir(const char* path) {
    bool ok = client.call("rmdir", path).as<bool>();
    if (!ok) {
        return -errno;
    }
    return 0;
}
