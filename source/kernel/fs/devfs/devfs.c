
#include "dev/dev.h"
#include "fs/devfs/devfs.h"
#include "fs/fs.h"
#include "tools/klib.h"
#include "tools/log.h"
#include "fs/file.h"
static  devfs_type_t  dev_type_table[] ={
    {
        .name = "tty",
        .dev_type = DEV_TTY,
        .file_type = FILE_TTY,
    },
};

int devfs_mount (struct _fs_t * fs, int major, int minor) {
    fs->type = FS_DEVFS;
    return 0;
}


void devfs_unmount (struct _fs_t * fs) {
}


int devfs_open (struct _fs_t * fs, const char * path, file_t * file) {   
    for(int i = 0; i < sizeof(dev_type_table)/sizeof(dev_type_table[0]); ++i)
    {
        devfs_type_t* type = dev_type_table + i;
        int len = kernel_strlen(type->name);
        if(kernel_strncmp(path, type->name, len) == 0)
        {
            int minor;
            if(kernel_strlen(path) > len && path_to_num(path + len, &minor) == 0)
            {
                int dev_id = dev_open(type->dev_type, minor, (void *)0);
                if (dev_id < 0) {
                    log_printf("Open device failed:%s", path);
                    break;
                }
                file->dev_id = dev_id;
                file->type = type->file_type;
                file->fs = fs;
                file->pos = 0;
                file->size = 0; 
                return 0;
            }
        }
    }
    return -1;
}


int devfs_read (char * buf, int size, file_t * file) {
    return dev_read(file->dev_id, file->pos, buf, size);
}


int devfs_write (char * buf, int size, file_t * file) {
    return dev_write(file->dev_id, file->pos, buf, size);
}


void devfs_close (file_t * file) {
    dev_close(file->dev_id);
}


int devfs_seek (file_t * file, uint32_t offset, int dir) {
    return -1;  
}


int devfs_stat(file_t * file, struct stat *st) {
    return -1;
}
int devfs_ioctl(file_t * file, int cmd, int arg0, int arg1) {
    
    return dev_control(file->dev_id, cmd, arg0, arg1);
}

fs_op_t devfs_op = {
    .mount = devfs_mount,
    .unmount = devfs_unmount,
    .open = devfs_open,
    .read = devfs_read,
    .write = devfs_write,
    .ioctl = devfs_ioctl,
    .seek = devfs_seek,
    .stat = devfs_stat,
    .close = devfs_close,
};