#ifndef _PWD_H
#define _PWD_H

#include <sys/types.h>

// moos has no users: getpwuid finds nobody, so xgalaga calls the player
// "Anonymous?"
typedef unsigned int uid_t;

struct passwd
{
  char *pw_name;
  char *pw_gecos;
};

static inline uid_t
getuid (void)
{
  return 0;
}

static inline struct passwd *
getpwuid (uid_t uid)
{
  return 0;
}

#endif
