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
  char *cwd;
};

static char *make_absolute_path(vfs_t *vfs, const char *name)
{
  int l;
  char *buf;
  const char *base = vfs->cwd;
  if (base == NULL || name[0] == '/')
    base = "/";
  buf = malloc((l = strlen(base))+strlen(name)+2);
  if (!buf)
    return NULL;
  memcpy(buf, base, l);
  for(;;) {
    while(*name == '/')
      name++;
    if (!*name)
      break;
    if (!l || buf[l-1] != '/')
      buf[l++] = '/';
    if (name[0] == '.' && (name[1] == '/' || name[1] == 0)) {
      name++;
      continue;
    } else if (name[0] == '.' && name[1] == '.' &&
	       (name[2] == '/' || name[2] == 0)) {
      if (l>1) {
	--l;
	while(l>0 && buf[l-1] != '/')
	  --l;
      }
      name+=2;
      continue;
    }
    while (*name && *name != '/')
      buf[l++] = *name++;
    if (*name == '/')
      buf[l++] = '/';
  }
  buf[l] = 0;
  return buf;
}

int vfs_stat(vfs_t *vfs, const char *name, vfs_stat_t *st)
{
  int offs, r = -ENOMEM;
  char *path = make_absolute_path(vfs, name);
  if (path) {
    vfsnode_t *vfsn = vfsnode_find(path, &offs);
    r = (vfsn? vfsnode_stat(vfsn, path+offs, st) : -ENOENT);
    free(path);
  }
  return r;
}

vfs_dirent_t *vfs_readdir(vfs_dir_t *dir)
{
  return vfsnode_readdir(dir);
}

vfs_dir_t *vfs_opendir(vfs_t *vfs, const char *name)
{
  int offs;
  vfs_dir_t *r = NULL;
  char *path = make_absolute_path(vfs, name);
  if (path) {
    vfsnode_t *vfsn = vfsnode_find(path, &offs);
    r = (vfsn? vfsnode_opendir(vfsn, path+offs) : NULL);
    free(path);
  }
  return r;
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
  char *apath = make_absolute_path(vfs, path);
  if (!apath)
    return -ENOMEM;
  if (vfs->cwd)
    free(vfs->cwd);
  vfs->cwd = apath;
  return 0;
}

char *vfs_getcwd(vfs_t *vfs, char *buf, size_t size)
{
  const char *cwd = vfs->cwd;
  if(!cwd)
    cwd = "/";
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
    vfs->cwd = NULL;
  }
  return vfs;
}

void vfs_closefs(vfs_t *vfs)
{
  if (vfs) {
    if (vfs->cwd)
      free(vfs->cwd);
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
