#ifndef LEONELOS_VFS_H
#define LEONELOS_VFS_H

#include <types.h>

typedef struct VNode VNode;
typedef struct File File;
typedef struct Directory Directory;
typedef struct FileSystem FileSystem;

struct File {
    VNode* node;
    usize offset;
    usize size;
    u32 flags;
    bool readable;
    bool writable;
};

struct Directory {
    VNode* node;
    usize entries;
};

struct FileSystem {
    const char* name;
    s32 (*mount)(VNode* root);
    s32 (*init)(void);
    void (*teardown)(void);
};

enum {
    VFS_OK = 0,
    VFS_ERR_NOT_FOUND = -1,
    VFS_ERR_PERMISSION = -2,
    VFS_ERR_INVALID = -3,
    VFS_ERR_IO = -4,
};

enum {
    VFS_FILE_READ = 1,
    VFS_FILE_WRITE = 2,
    VFS_FILE_EXEC = 4,
};

enum {
    VFS_NODE_FILE = 1,
    VFS_NODE_DIRECTORY = 2,
};

s32 vfs_init();
s32 vfs_mount_root(const char* fs_type);
File* vfs_open(const char* path, u32 flags);
s32 vfs_close(File* file);
s64 vfs_read(File* file, void* buffer, usize size);
s64 vfs_write(File* file, const void* buffer, usize size);
s64 vfs_seek(File* file, usize offset, u32 whence);
s32 vfs_mkdir(const char* path);
s32 vfs_unlink(const char* path);
s32 vfs_list_dir(const char* path, char*** names, usize* count);
const char* vfs_strerror(s32 err);

#define VFS_READ SEEK_SET
#define VFS_WRITE SEEK_CUR
#define VFS_END  SEEK_END

#endif