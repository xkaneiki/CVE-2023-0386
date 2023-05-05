// gcc fuse.c -o test -D_FILE_OFFSET_BITS=64 -static -pthread -lfuse -ldl
#define FUSE_USE_VERSION 29
#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
#include <pthread.h>

int fpair[2]; // 用于进程间通信
int pause;

void fatal(const char *msg)
{
    perror(msg);
    exit(1);
}

char content[0x1000];
int clen;
static char mount_path[] = "./ovlcap/lower3";
static int cnt = 0;

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

int access_callback(const char *fn, int flag)
{
    puts("[+] access_callback");
    return 0;
}

int release_callback(const char *fn, struct fuse_file_info *fi)
{
    puts("[+] release_callback");
    puts(fn);
    return 0;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    puts("[+] readdir");
    filler(buf, "file", NULL, 0);
    return 0;
}
static int statfs_callback(const char *path, struct statvfs *stvfs)
{
    // Set the file system statistics
    stvfs->f_bsize = 4096;
    stvfs->f_blocks = 10000;
    stvfs->f_bfree = 8000;
    stvfs->f_bavail = 8000;
    stvfs->f_files = 1000;
    stvfs->f_ffree = 900;
    stvfs->f_favail = 900;
    stvfs->f_fsid = 0;
    stvfs->f_flag = 0;
    stvfs->f_namemax = 255;

    return 0;
}

static int utimens_callback(const char *path, const struct timespec tv[2])
{
    puts("[+] utimens_callback");
    return 0;
}

static int create_callback(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    puts("[+] create_callback");
    return 0;
}

static int write_callback(const char *path,
                          char *buf, size_t size, off_t offset,
                          struct fuse_file_info *fi)
{
    puts("[+] write callback");
    return 0;
}

static int truncate_callback(const char *path, off_t size)
{
    puts("[+] truncate_callback");
    return 0;
}

static int mknod_callback(const char *path, mode_t mode, dev_t dev)
{
    puts("[+] mknod_callback");
    return 0;
}

static int fgetattr_callback(const char *path, struct stat *st, struct fuse_file_info *fi)
{
    puts("[+] fgetattr_callback");
    return 0;
}

static int flush_callback(const char *path, struct fuse_file_info *fi)
{
    puts("[+] flush_callback");
    return 0;
}

static int opendir_callback(const char *path, struct fuse_file_info *fi)
{
    puts("[+] opendir_callback");
    return 0;
}

static int readlink_callback(const char *path, char *buf, size_t size)
{
    puts("[+] readlink_callback");
    return 0;
}

static int mkdir_cb(const char *path, mode_t mode)
{
    puts("[+] mkdir");
    return 0;
}
static int unlink_cb(const char *path)
{
    puts("[+] unlink");
    return 0;
}
static int rmdir_cb(const char *path)
{
    puts("[+] rmdir");
    return 0;
}
static int symlink_cb(const char *path1, const char *path2)
{
    puts("[+] symblink");
    return 0;
}
static int rename_cb(const char *p1, const char *p2)
{
    puts("[+] rename");
    return 0;
}
static int link_cb(const char *p1, const char *p2)
{
    puts("[+] link");
    return 0;
}
static int chmod_cb(const char *p, mode_t m)
{
    puts("[+] chmod");
    return 0;
}
static int chown_cb(const char *p, uid_t u, gid_t g)
{
    puts("[+] chown");
    return 0;
}
static int fsync_cb(const char *p, int, struct fuse_file_info *fi)
{
    puts("[+] fsync");
    return 0;
}
static int setxattr_cb(const char *p, const char *p1, const char *p2, size_t size, int i)
{
    puts("[+] setxattr");
    return 0;
}
static int getxattr_cb(const char *p1, const char *p2, char *p3, size_t p4)
{
    puts("[+] getxattr");
    return 0;
}
static int listxattr_cb(const char *p1, char *p2, size_t p3)
{
    puts("[+] listxattr");
    return 0;
}
static int removexattr_cb(const char *p1, const char *p2)
{
    puts("[+] removexattr");
    return 0;
}
static int releasedir_cb(const char *p1, struct fuse_file_info *fi)
{
    puts("[+] releasedir");
    return 0;
}
static int fsyncdir_cb(const char *p1, int p2, struct fuse_file_info *fi)
{
    puts("[+] fsyncdir");
    return 0;
}

// static void *(*init)(struct fuse_conn_info *conn);
// static void (*destroy)(void *);
static int ftruncate_cb(const char *p1, off_t off, struct fuse_file_info *fi)
{
    puts("[+] ftruncate_cb");
    return 0;
}
static int lock_cb(const char *path, struct fuse_file_info *fi, int cmd, struct flock *fl)
{
    puts("[+] lock_cb");
    return 0;
}
static int bmap_cb(const char *p, size_t blocksize, uint64_t *idx)
{
    puts("[+] bmap_cb");
    return 0;
}
static int ioctl_cb(const char *p, int cmd, void *arg,
                    struct fuse_file_info *fi, unsigned int flags, void *data)
{
    puts("[+] ioctl cb");
    printf("path %s\n", p);
    printf("cmd 0x%x\n", cmd);
    return 0;
}
static int poll_cb(const char *p, struct fuse_file_info *fi,
                   struct fuse_pollhandle *ph, unsigned *reventsp)
{
    puts("[+] poll cb");
    return 0;
}
static int write_buf_cb(const char *p1, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *fi)
{
    puts("[+] write_buf");
    return 0;
}

int read_buf_cb(const char *path, struct fuse_bufvec **bufp,
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
    src->buf[0].flags = FUSE_BUF_FD_SEEK;
    src->buf[0].pos = off;
    src->buf[0].mem = data;

    *bufp = src;

    return 0;
}
static int flock_cb(const char *p, struct fuse_file_info *fi, int op)
{
    puts("[+] flock cb");
    return 0;
}
static int fallocate_cb(const char *p, int, off_t p1, off_t p2, struct fuse_file_info *fi)
{
    puts("[+] fallocate_cb");
    return 0;
}

static struct fuse_operations fops = {
    .getattr = getattr_callback,
    .open = open_callback,
    .read = read_callback,
    .access = access_callback,
    .release = release_callback,
    .readdir = readdir_callback,
    .statfs = statfs_callback,
    .utimens = utimens_callback,
    .create = create_callback,
    .write = write_callback,

    .truncate = truncate_callback,
    .mknod = mknod_callback,
    .fgetattr = fgetattr_callback,
    .flush = flush_callback,
    .opendir = opendir_callback,
    .readlink = readlink_callback,

    .mkdir = mkdir_cb,
    .unlink = unlink_cb,
    .rmdir = rmdir_cb,
    .symlink = symlink_cb,
    .rename = rename_cb,
    .link = link_cb,
    .chmod = chmod_cb,
    .chown = chown_cb,
    .fsync = fsync_cb,
    .setxattr = setxattr_cb,
    .getxattr = getxattr_cb,
    .listxattr = listxattr_cb,
    .removexattr = removexattr_cb,
    .releasedir = releasedir_cb,
    .fsyncdir = fsyncdir_cb,

    .read_buf = read_buf_cb,
    .write_buf = write_buf_cb,

    .ftruncate = ftruncate_cb,
    .lock = lock_cb,
    .bmap = bmap_cb,
    .ioctl = ioctl_cb,
    .poll = poll_cb,
    .flock = flock_cb,
    .fallocate = fallocate_cb

};

void start_fuse()
{
    // mk mount path
    if (mkdir(mount_path, 0777))
        perror("mkdir");

    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    struct fuse_chan *chan;
    struct fuse *fuse;

    // fuse_opt_add_arg(&args, "./test");
    // fuse_opt_add_arg(&args, "-o");
    // fuse_opt_add_arg(&args, "allow_root");
    // fuse_opt_add_arg(&args, mount_path);

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
    strcpy(content, "hello world!");
    clen = strlen(content);
    start_fuse();
    rmdir(mount_path);
    // getchar();
    return 0;
}
