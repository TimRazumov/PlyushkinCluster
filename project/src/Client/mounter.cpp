// Hello filesystem class implementation

#include "mounter.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <rpc/rpc_error.h>

// include in one .cpp file
#include "Fuse-impl.h"

#include "utils.hpp"

const int TIMEOUT = 5000;

using err_t = std::tuple<int, std::string>;


void* Mounter::init(struct fuse_conn_info*, struct fuse_config*) {
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init");
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        client.call("init");
    } catch (rpc::timeout) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init timeout error");
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
    }
}

int Mounter::access(const char* path, int) {
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "access: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        int ret = client.call("access", path).as<int>();
        if (ret == -1) {
            return -errno;
        }
        return 0;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "access timeout error");
        return -errno;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EACCES;
    }
    
}

int Mounter::getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
{
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "getattr: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        auto tmp = client.call("getattr", path).as<std::vector<unsigned int>>();
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
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "getattr timeout error");
        return -EBADF;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EBADF;
    }
}

int Mounter::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "readdir: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        if (!client.call("isDir", path).as<bool>())
            return -ENOENT;

        auto tmp = client.call("readdir", path).as<std::vector<std::string>>();
        filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
        filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
        for (int i = 0; i < tmp.size(); i++) {
            filler(buf, tmp[i].c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
        }
        return 0;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "readdir timeout error");
        return -EBADF;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EBADF;
    }
}


int Mounter::open(const char *path, struct fuse_file_info *fi)
{
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "open: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        if (!(client.call("isFile", path).as<bool>())) {
            return -ENOENT;
        }
        return 0;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "open timeout error");
        return -EFAULT;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EFAULT;
    }
}


int Mounter::read(const char *path, char *buf, size_t size, off_t offset,
		              struct fuse_file_info *)
{
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "read: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        if (!(client.call("isFile", path).as<bool>()))
		    return -ENOENT;

        auto data = client.call("read", path, size, offset).as<std::vector<char>>();
        size = data.size();
        memcpy(buf, data.data(), size);
	    return size;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "read timeout error");
        return -EAGAIN;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EAGAIN;
    }
}

int Mounter::write(const char *path, const char *buf, size_t size,
                   off_t offset, struct fuse_file_info *) {
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "write: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        std::vector<char> my_buf(size);
        for (size_t i = 0; i < size; i++) {
            my_buf[i] = buf[i];
        }
        int len = client.call("write", path,
                              my_buf, size, offset).as<int>();
        return len;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "write timeout error");
        return -EAGAIN;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EAGAIN;
    }
}

int Mounter::mknod(const char *path, mode_t mode, dev_t) {
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "mknod: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        std::vector<unsigned int> attrs = {0, 1};
        auto perms = get_perm(mode);
        attrs.insert(attrs.end(), perms.begin(), perms.end());
        bool ok = client.call("mknod", path, attrs).as<bool>();
        if (!ok) {
            return -errno;
        }
        return 0;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "mknod timeout error");
        return -EACCES;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        return -EACCES;
    }
}

int Mounter::unlink(const char *path) {
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "unlink: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        std::cout << "unlink: " << path << std::endl;
        bool ok = client.call("delete_file", path).as<bool>();
        if (!ok) {
            return -errno;
        }
        return 0;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "unlink timeout error");
        return -EFAULT;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EFAULT;
    }
}

int Mounter::rename(const char* from, const char* to, unsigned int) {
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "rename: " +
                std::string(from) + " -> " + std::string(to));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        bool ok = client.call("rename", from, to).as<bool>();
        if (!ok) {
            return -errno;
        }
        return 0;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "rename timeout error");
        return -EFAULT;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EFAULT;
    }
}

int Mounter::mkdir(const char *path, mode_t mode) {
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "mkdir: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        std::vector<unsigned int> attrs = {0, 2};
        auto perms = get_perm(mode);
        attrs.insert(attrs.end(), perms.begin(), perms.end());
        bool ok = client.call("mknod", path, attrs).as<bool>();
        if (!ok) {
            return -errno;
        }
        return 0;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "mkdir timeout error");
        return -EFAULT;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        return -EFAULT;
    }
}

int Mounter::rmdir(const char* path) {
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "rmdir: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        bool ok = client.call("delete_file", path).as<bool>();
        if (!ok) {
            return -errno;
        }
        return 0;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "rmdir timeout error");
        return -EFAULT;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EFAULT;
    }
}

int Mounter::chmod(const char* path, mode_t mode, struct fuse_file_info*) {
    try {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "chmod: " + std::string(path));
        rpc::client client("127.0.0.1", 2280);
        client.set_timeout(TIMEOUT);
        if (!client.call("chmod", path, get_perm(mode)).as<bool>())
            return -errno;
        return 0;
    } catch (rpc::timeout& T) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "chmod timeout error");
        return -EFAULT;
    } catch (rpc::rpc_error &e) {
        add_log(getenv("HOME") + std::string("/plyushkincluster/fuse/"), "init rpc error");
        auto err = e.get_error().as<err_t>();
        if (std::get<0>(err) == 3) {
            return -ENOENT;
        }
        return -EFAULT;
    }
}

//TODO: realize this method
int Mounter::utimens(const char*, const struct timespec*, struct fuse_file_info *) {
    return 0;
}
