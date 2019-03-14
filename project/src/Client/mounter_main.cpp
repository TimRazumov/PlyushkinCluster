// See  FUSE:  example/hello.c

#include "mounter.h"
#include <rpc/client.h>

int main(int argc, char *argv[])
{

  Mounter fs;
  
  int status = fs.run(argc, argv);

  return status;
}
