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

#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "vfs.h"
#include "vfsnode.h"

struct vfs_s {
  int dummy;
};

int vfs_stat(vfs_t *vfs, const char *name, vfs_stat_t *st)
{
  int offs;
  vfsnode_t *vfsn = vfsnode_find(name, &offs);
  return (vfsn? vfsnode_stat(vfsn, name+offs, st) : -ENOENT);
}

vfs_dirent_t *vfs_readdir(vfs_dir_t *dir)
{
  return vfsnode_readdir(dir);
}

vfs_dir_t *vfs_opendir(vfs_t *vfs, const char *path)
{
  int offs;
  vfsnode_t *vfsn = vfsnode_find(path, &offs);
  return (vfsn? vfsnode_opendir(vfsn, path+offs) : NULL);
}

int vfs_closedir(vfs_dir_t *dir)
{
  return vfsnode_closedir(dir);
}

vfs_file_t *vfs_open(vfs_t *vfs, const char *path, const char *mode)
{
  return NULL;
}

int vfs_read(void *buffer, size_t size, size_t nmemb, vfs_file_t *file)
{
  return -ENOSYS;
}

int vfs_write(const void *buffer, size_t size, size_t nmemb, vfs_file_t *file)
{
  return -ENOSYS;
}

int vfs_eof(vfs_file_t *file)
{
  return -ENOSYS;
}

int vfs_close(vfs_file_t *file)
{
  return -ENOSYS;
}

int vfs_chdir(vfs_t *vfs, const char *path)
{
  return -ENOSYS;
}

char *vfs_getcwd(vfs_t *vfs, char *buf, size_t size)
{
  const char *cwd = "/";
  if (buf) {
    if (strlen(cwd) >= size) {
      if (size>1)
	memcpy(buf, cwd, size-1);
      if (size>0)
	buf[size-1] = 0;
    } else
      strcpy(buf, cwd);
    return buf;
  } else
    return strdup(cwd);
}

int vfs_rename(vfs_t *vfs, const char *frompath, const char *topath)
{
  return -ENOSYS;
}

int vfs_mkdir(vfs_t *vfs, const char *path, int mode)
{
  return -ENOSYS;
}

int vfs_rmdir(vfs_t *vfs, const char *path)
{
  return -ENOSYS;
}

int vfs_remove(vfs_t *vfs, const char *path)
{
  return -ENOSYS;
}

vfs_t *vfs_openfs(void)
{
  vfs_t *vfs = calloc(1, sizeof(vfs_t));
  if (vfs) {
    vfs->dummy = 0;
  }
  return vfs;
}

void vfs_closefs(vfs_t *vfs)
{
  if (vfs) {
    free(vfs);
  }
}

void vfs_load_plugin(int id)
{
}

void vfs_init(void)
{
  vfsnode_init();
}
