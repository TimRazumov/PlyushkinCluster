// See  FUSE:  example/hello.c

#include "mounter.h"
#include <rpc/client.h>

int main(int argc, char *argv[])
{

  HelloFS fs;
  
  int status = fs.run(argc, argv);

  return status;
}
