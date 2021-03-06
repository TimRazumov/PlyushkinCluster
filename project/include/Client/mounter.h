// Hello filesystem class definition

#ifndef __HELLOFS_H_
#define __HELLOFS_H_

#include "Fuse_cls.h"
#include "rpc/client.h"
#include "boost/filesystem.hpp"

#include "Fuse-impl.h"

class Mounter : public Fusepp::Fuse<Mounter>
{
public:
  Mounter() {
    boost::filesystem::create_directories(getenv("HOME") + std::string("/plyushkincluster/fuse/"));
  }

  ~Mounter() {}
  
  static int access(const char*, int);

  static void* init(struct fuse_conn_info*, struct fuse_config*);

  static int getattr (const char *, struct stat *, struct fuse_file_info *);

  static int readdir(const char *path, void *buf,
                     fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi,
                     enum fuse_readdir_flags);
  
  static int open(const char *path, struct fuse_file_info *fi);

  static int read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi);
  
  static int mknod(const char *, mode_t, dev_t);

  static int write(const char *, const char*, size_t, off_t,
                   struct fuse_file_info *);

  static int unlink(const char *);

  static int rename(const char *, const char *, unsigned int);

  static int mkdir(const char *, mode_t);

  static int rmdir(const char *);

  static int chmod(const char*, mode_t, struct fuse_file_info*);

  static int utimens(const char*, const struct timespec*, struct fuse_file_info *);  
};

#endif
