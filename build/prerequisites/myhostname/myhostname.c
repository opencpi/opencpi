// This is a prerequisite so that users of it do not depend on the framework being built
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int gethostname(char *name, size_t len) {
  const char *myhostname = getenv("MYHOSTNAME");
  if (!myhostname || len <= strlen(myhostname))
    return EINVAL;
  strcpy(name, myhostname);
  return 0;
}
