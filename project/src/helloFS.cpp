// Hello filesystem class implementation

#include "helloFS.h"

#include <iostream>
#include <vector>
#include <string>
#include <bitset>

// include in one .cpp file
#include "Fuse-impl.h"

const int TIMEOUT = 10000;

rpc::client client("127.0.0.1", 2280);

const std::vector<mode_t> perms_t = {
    0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007
};

std::vector<unsigned int> get_perm(mode_t mode) {
    auto o_perm = std::bitset<3>(mode);
    auto g_perm = std::bitset<3>(mode >> 3);
    auto u_perm = std::bitset<3>(mode >> 6);
    unsigned int o_id = 0;
    unsigned int g_id = 0;
    unsigned int u_id = 0;
    for (int i = 0; i < perms_t.size(); i++) {
        if (o_perm == std::bitset<3>(perms_t[i]))
            o_id = i;
        if (g_perm == std::bitset<3>(perms_t[i]))
            g_id = i;
        if (u_perm == std::bitset<3>(perms_t[i]))
            u_id = i;
    }
    std::vector<unsigned int> ret = {o_id, g_id, u_id};
    return ret;
}

void* HelloFS::init(struct fuse_conn_info*, struct fuse_config*) {
    client.set_timeout(TIMEOUT);
    client.call("init");
}

int HelloFS::access(const char* path, int) {
    std::cout << "access: " << path << std::endl;
    int ret = client.call("access", path).as<int>();
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

int HelloFS::getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
{
    std::cout << "getattr: " << path << std::endl;
    auto tmp = client.call("getattr", path).as<std::vector<unsigned int>>();
    if (tmp.empty()) {
        return -ENOENT;
    }
	memset(stbuf, 0, sizeof(struct stat));
	if (tmp[1] > 1) {
		stbuf->st_mode = S_IFDIR |
            (perms_t[tmp[2]] | perms_t[tmp[3]] << 3 | perms_t[tmp[4]] << 6);
		stbuf->st_nlink = tmp[1];
        stbuf->st_size = 4096;
	} else {
		stbuf->st_mode = S_IFREG |
            (perms_t[tmp[2]] | perms_t[tmp[3]] << 3 | perms_t[tmp[4]] << 6);
		stbuf->st_nlink = tmp[1];
		stbuf->st_size = tmp[0];
    }
	return 0;
}

int HelloFS::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
    std::cout << "readdir: " << path << std::endl;
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
    std::cout << "open: " << path << std::endl;
	if (!(client.call("isFile", path).as<bool>())) {
		return -ENOENT;
    }
	return 0;
}


int HelloFS::read(const char *path, char *buf, size_t size, off_t offset,
		              struct fuse_file_info *)
{
    std::cout << "read: " << path << std::endl;
	if (!(client.call("isFile", path).as<bool>()))
		return -ENOENT;

	auto data = client.call("read", path, size, offset).as<std::vector<char>>();
	size = data.size();
    memcpy(buf, data.data(), size);

	return size;
}

int HelloFS::write(const char *path, const char *buf, size_t size,
                   off_t offset, struct fuse_file_info *) {
    std::cout << "write: " << path << " " << buf << " "
              << size << " " << offset << std::endl;
    std::vector<char> my_buf(size);
    for (size_t i = 0; i < size; i++) {
        my_buf[i] = buf[i];
    }
    int len = client.call("write", path,
                          my_buf, size, offset).as<int>();
    return len;
}

int HelloFS::mknod(const char *path, mode_t mode, dev_t) {
    std::cout << "mknod: " << path << std::endl;
    std::vector<unsigned int> attrs = {0, 1};
    auto perms = get_perm(mode);
    attrs.insert(attrs.end(), perms.begin(), perms.end());
    bool ok = client.call("mknod", path, attrs).as<bool>();
    if (!ok) {
        return -errno;
    }
    return 0;
}

int HelloFS::unlink(const char *path) {
    std::cout << "unlink: " << path << std::endl;
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

int HelloFS::mkdir(const char *path, mode_t mode) {
    std::cout << "mkdir: " << path << std::endl;
    std::vector<unsigned int> attrs = {0, 2};
    auto perms = get_perm(mode);
    attrs.insert(attrs.end(), perms.begin(), perms.end());
    bool ok = client.call("mknod", path, attrs).as<bool>();
    if (!ok) {
        return -errno;
    }
    return 0;
}

int HelloFS::rmdir(const char* path) {
    std::cout << "rmdir: " << path << std::endl;
    bool ok = client.call("delete_file", path).as<bool>();
    if (!ok) {
        return -errno;
    }
    return 0;
}

int HelloFS::chmod(const char* path, mode_t mode, struct fuse_file_info*) {
    std::cout << "chmod: " << path << std::endl;
    if (!client.call("chmod", path, get_perm(mode)).as<bool>())
        return -errno;
    return 0;
}

int HelloFS::utimens(const char*, const struct timespec*, struct fuse_file_info *) {
    return 0;
}
