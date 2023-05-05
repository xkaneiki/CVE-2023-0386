#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mount.h>

int main(int argc, char const *argv[])
{
    int ret;
    ret = setuid(0);
    if (ret < 0)
    {
        perror("setuid");
        return -1;
    }
    ret = setgid(0);
    if (ret < 0)
    {
        perror("setgid");
        return -1;
    }
    system("/bin/bash");
    return 0;
}
