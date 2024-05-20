#include "fs/fs.h"
#include "tools/klib.h"
#include "dev/console.h"
#include "fs/file.h"
#include "cpu/irq.h"
#include "core/task.h"
#include "core/memory.h"
#include "ipc/mutex.h"
#include "tools/list.h"
#include "fs/devfs/devfs.h"
#include "dev/disk.h"
#include "os_cfg.h"
#define     FS_TABLE_SIZE       10
static list_t   mounted_list;
static fs_t     fs_table[FS_TABLE_SIZE];
static list_t   free_list;
static fs_t*     root_fs;
extern fs_op_t devfs_op;
extern fs_op_t fatfs_op;

int path_to_num (const char * path, int * num) {
	int n = 0;

	const char * c = path;
	while (*c && *c != '/') {
		n = n * 10 + *c - '0';
		c++;
	}
	*num = n;
	return 0;
}

const char * path_next_child (const char * path) {
   const char * c = path;

    while (*c && (*c++ == '/')) {}
    while (*c && (*c++ != '/')) {}
    return *c ? c : (const char *)0;
}
int path_begin_with (const char * path, const char * str) {
	const char * s1 = path, * s2 = str;
	while (*s1 && *s2 && (*s1 == *s2)) {
		s1++;
		s2++;
	}

	return *s2 == '\0';
}

static int is_valid_path(char * path)
{
    if(path == (char*)0 || path[0] == '\0')
    {
        return 0;
    }
    return 1;
}
void fs_protect(fs_t* fs)
{
    if(fs->mutex)
        mutex_lock(fs->mutex);
}
void fs_unprotect(fs_t* fs)
{
    if(fs->mutex)
        mutex_unlock(fs->mutex);
}
int sys_open(char* name, int flag, ...)
{   

    file_t* file = file_alloc();
    if(file == (file_t*)0)
    {
        log_printf("file table err");
        goto open_failed;
    }
    task_t* task = task_manager.curr_task;
    int fd = -1;
    fd = task_alloc_fd(file);
    if(fd < 0)
    {
        log_printf("task file table err");
        goto open_failed;
    }
    
    list_node_t* node = list_first(&mounted_list);
    fs_t* fs = (fs_t*)0;
    while(node)
    {
        fs_t* curr = LIST_NODE_PARENT(node, fs_t, node);
        if(path_begin_with(name, curr->mount_point))
        {
            fs = curr;
            break;
        }
        node = list_node_next(node);
    }
    if(fs)
    {
        name = path_next_child(name);
    }
    else{
        fs = root_fs;
    }
    file->fs = fs;    
    file ->ref = 1;
    file->mode = flag;
    kernel_strncpy(file->file_name, name, FILE_NAME_SIZE);
    fs_protect(fs);
    int err = fs->op->open(fs, name, file);
    if (err < 0) {
		fs_unprotect(fs);
		log_printf("open %s failed.", name);
		return -1;
	}
    fs_unprotect(fs);
    return fd;
open_failed:
    if(file)
        file_free(file);
    if(fd)
        task_remove_fd(fd);
    return -1;

}
    
int sys_read(int file, char* buf, int size)
{   
  
    if(is_file_bad(file))
	{
        log_printf("file(%d) is not valid.", file);
		return -1;
	}
    file_t * p_file = task_file(file);
    if (!p_file) {
        log_printf("file not opened");
        return -1;
    }
 
    fs_t* fs = p_file->fs;
    return fs->op->read(buf, size, p_file);
    
}
int sys_write(int file, char* buf, int size)
{
    // file = 0;		

    file_t * p_file = task_file(file);
    if (!p_file) {
        log_printf("file not opened");
        return -1;
    }

    fs_t* fs = p_file->fs;
    return fs->op->write(buf, size, p_file);

}
int sys_lseek(int file, int ptr, int dir){
    

    if(is_file_bad(file))
	{
        log_printf("file(%d) is not valid.", file);
		return -1;
	}
    file_t * p_file = task_file(file);
    if (!p_file) {
        log_printf("file not opened");
        return -1;
    }
    fs_t* fs = p_file->fs;
    return fs->op->seek(p_file, ptr, dir);
    
}
int sys_close(int file){


  
    if(is_file_bad(file))
	{
        log_printf("file(%d) is not valid.", file);
		return -1;
	}
    file_t * p_file = task_file(file);
    if (!p_file) {
        log_printf("file not opened");
        return -1;
    }
    Assert(p_file->ref > 0);
    if (p_file->ref-- == 1) {
		fs_t * fs = p_file->fs;

		fs_protect(fs);
		fs->op->close(p_file);
		fs_unprotect(fs);
	    file_free(p_file);
	}

	task_remove_fd(file);
    return 0;
}



int sys_isatty(int file) {
	if(is_file_bad(file))
	{
        log_printf("file(%d) is not valid.", file);
		return -1;
	}
    file_t * p_file = task_file(file);
    if (!p_file) {
        log_printf("file not opened");
        return -1;
    }
    return p_file->type == FILE_TTY;
}



int sys_fstat(int file, struct stat *st) {
    if(is_file_bad(file))
	{
        log_printf("file(%d) is not valid.", file);
		return -1;
	}
    file_t * p_file = task_file(file);
    if (!p_file) {
        log_printf("file not opened");
        return -1;
    }
    kernel_memset(st, 0, sizeof(struct stat));
    fs_t* fs = p_file->fs;
    fs->op->stat(p_file, st);
    return -1;
}
static void mount_list_init(void)
{
    list_init(&mounted_list);
    kernel_memset((void*)fs_table, 0, FS_TABLE_SIZE);
    list_init(&free_list);
    for(int i = 0; i < FS_TABLE_SIZE; ++i)
    {
        list_insert_first(&free_list, &fs_table[i].node);
    }
}
fs_op_t* get_fs_op(fs_type_t type, int major)
{
    switch (type)
    {
    case FS_FAT16:
        return &fatfs_op;
    case FS_DEVFS:
        return &devfs_op;
 
    default:
        log_printf("unknown fs type\n");
        break;
    }
    return (fs_op_t*)0;
}

fs_t* mount(fs_type_t type, char* mount_point, int major, int minor )
{
    fs_t* fs = (fs_t*)0;
    log_printf("mount file system, name: %s, dev: %x", mount_point, major);
    list_node_t * curr = list_first(&mounted_list);
	while (curr) {
		fs_t * fs = LIST_NODE_PARENT(curr, fs_t, node);
		if (kernel_strncmp(fs->mount_point, mount_point, FS_MOUNTP_SIZE) == 0) {
			log_printf("fs alreay mounted.");
			goto mount_failed;
		}
		curr = list_node_next(curr);
	}
    list_node_t * free_node = list_remove_first(&free_list);
	if (!free_node) {
		log_printf("no free fs, mount failed.");
		goto mount_failed;
	}
    fs = LIST_NODE_PARENT(free_node, fs_t, node);
    fs_op_t* op = get_fs_op(type, major);
    kernel_memset(fs, 0, sizeof(fs_t));
	kernel_strncpy(fs->mount_point, mount_point, FS_MOUNTP_SIZE);
	fs->op = op;
	fs->mutex = (mutex_t *)0;
    if (op->mount(fs, major, minor) < 0) {
		log_printf("mount fs %s failed", mount_point);
		goto mount_failed;
	}
    list_insert_first(&mounted_list, &fs->node);
    return fs;
mount_failed:
	if (fs) {
		list_insert_first(&free_list, &fs->node);
	}
}
void fs_init(void)
{
    file_table_init();
    mount_list_init();
    disk_init();
    fs_t* fs = mount(FS_DEVFS, "/dev", 0, 0);
    root_fs = mount(FS_FAT16, "/home", ROOT_DEV );
}
static int is_file_bad(int file)
{
    if ((file < 0) || (file >= TASK_OFILE_NR))
        return 1;
    return 0;
}
int sys_ioctl(int file, int cmd, int arg0, int arg1)
{
    if(is_file_bad(file))
	 {
        log_printf("file(%d) is not valid.", file);
		return -1;
	}

	file_t * p_file = task_file(file);
	if (!p_file) {
		log_printf("file not opened");
		return -1;
	}
    fs_t* fs = p_file->fs;
    fs_protect(root_fs);
    int err = fs->op->ioctl(p_file, cmd, arg0, arg1);
    fs_unprotect(root_fs);
    return err;
}
int sys_dup (int file) {
	if(is_file_bad(file))
	 {
        log_printf("file(%d) is not valid.", file);
		return -1;
	}

	file_t * p_file = task_file(file);
	if (!p_file) {
		log_printf("file not opened");
		return -1;
	}

	int fd = task_alloc_fd(p_file);	
	if (fd >= 0) {
		ref_inc(p_file);		
		return fd;
	}

	log_printf("No task file avaliable");
    return -1;
}

int sys_opendir(const char* name, DIR* dir)
{
    fs_protect(root_fs);
    int err = root_fs->op->opendir(root_fs, name, dir);
    fs_unprotect(root_fs);
    return err;
}
int sys_readdir(DIR* dir, struct dirent* dirent)
{
    fs_protect(root_fs);
    int err = root_fs->op->readdir(root_fs, dir, dirent);
    fs_unprotect(root_fs);
    return err;
}
int sys_closedir(DIR* dir)
{
    fs_protect(root_fs);
    int err = root_fs->op->closedir(root_fs, dir);
    fs_unprotect(root_fs);
    return err;
}

int sys_unlink(const char* name)
{
    fs_protect(root_fs);
    int err = root_fs->op->unlink(root_fs, name);
    fs_unprotect(root_fs);
    return err;
}