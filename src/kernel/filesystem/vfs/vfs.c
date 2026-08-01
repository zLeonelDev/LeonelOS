#include <vfs.h>
#include <debug.h>
#include <physical.h>

static VNode* g_root = NULL;

s32 vfs_init() {
    debug_log("VFS initialized\n");
    return VFS_OK;
}

s32 vfs_mount_root(const char* fs_type) {
    UNUSED(fs_type);
    return VFS_OK;
}

File* vfs_open(const char* path, u32 flags) {
    UNUSED(path);
    UNUSED(flags);
    return NULL;
}

s32 vfs_close(File* file) {
    UNUSED(file);
    return VFS_OK;
}

s64 vfs_read(File* file, void* buffer, usize size) {
    UNUSED(file);
    UNUSED(buffer);
    UNUSED(size);
    return 0;
}

s64 vfs_write(File* file, const void* buffer, usize size) {
    UNUSED(file);
    UNUSED(buffer);
    UNUSED(size);
    return 0;
}

s64 vfs_seek(File* file, usize offset, u32 whence) {
    UNUSED(file);
    UNUSED(offset);
    UNUSED(whence);
    return 0;
}

s32 vfs_mkdir(const char* path) {
    UNUSED(path);
    return VFS_OK;
}

s32 vfs_unlink(const char* path) {
    UNUSED(path);
    return VFS_OK;
}

s32 vfs_list_dir(const char* path, char*** names, usize* count) {
    UNUSED(path);
    UNUSED(names);
    UNUSED(count);
    return VFS_OK;
}

const char* vfs_strerror(s32 err) {
    switch (err) {
        case VFS_OK:             return "Success";
        case VFS_ERR_NOT_FOUND:  return "Not found";
        case VFS_ERR_PERMISSION: return "Permission denied";
        case VFS_ERR_INVALID:    return "Invalid argument";
        case VFS_ERR_IO:         return "I/O error";
        default:                 return "Unknown error";
    }
}