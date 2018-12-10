// Hello filesystem class definition

#ifndef __HELLOFS_H_
#define __HELLOFS_H_

#include "Fuse.h"
#include "rpc/client.h"
#include "Fuse-impl.h"

class HelloFS : public Fusepp::Fuse<HelloFS>
{
public:
  HelloFS() {}

  ~HelloFS() {}

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
  
};

#endif
