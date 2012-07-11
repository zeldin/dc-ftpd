/*
 * Copyright (c) 2012 Marcus Comstedt.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef __VFS_H__
#define __VFS_H__

typedef struct vfs_dir_s vfs_dir_t;
typedef struct vfs_dirent_s vfs_dirent_t;
typedef struct vfs_file_s vfs_file_t;
typedef struct vfs_stat_s vfs_stat_t;
typedef struct vfs_s vfs_t;

struct vfs_dirent_s {
  void *private;
  char name[];
};

struct vfs_stat_s {
  int st_mode;
  time_t st_mtime;
  size_t st_size;
};

int vfs_stat(vfs_t *vfs, const char *name, vfs_stat_t *st);
vfs_dirent_t *vfs_readdir(vfs_dir_t *dir);
vfs_dir_t *vfs_opendir(vfs_t *vfs, const char *path);
int vfs_closedir(vfs_dir_t *dir);
vfs_file_t *vfs_open(vfs_t *vfs, const char *path, const char *mode);
int vfs_read(void *buffer, size_t size, size_t nmemb, vfs_file_t *file);
int vfs_write(const void *buffer, size_t size, size_t nmemb, vfs_file_t *file);
int vfs_eof(vfs_file_t *file);
int vfs_close(vfs_file_t *file);
int vfs_chdir(vfs_t *vfs, const char *path);
char *vfs_getcwd(vfs_t *vfs, char *buf, size_t size);
int vfs_rename(vfs_t *vfs, const char *frompath, const char *topath);
int vfs_mkdir(vfs_t *vfs, const char *path, int mode);
int vfs_rmdir(vfs_t *vfs, const char *path);
int vfs_remove(vfs_t *vfs, const char *path);
vfs_t *vfs_openfs(void);
void vfs_closefs(vfs_t *vfs);
void vfs_load_plugin(int id);

#define VFS_ISDIR(x) (x)
#define VFS_ISREG(x) (!(x))

#define VFS_IRWXU 0
#define VFS_IRWXG 0
#define VFS_IRWXO 0

#define vfs_default_fs 0

#endif				/* __VFS_H__ */
