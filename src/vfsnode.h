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

#ifndef __VFSNODE_H__
#define __VFSNODE_H__

typedef struct vfsnode_s vfsnode_t;
typedef struct vfsnode_vtable_s vfsnode_vtable_t;

struct vfsnode_vtable_s
{
  void (*init)(vfsnode_t *, void *);
  void (*add_child)(vfsnode_t *, vfsnode_t *);
  vfsnode_t *(*find)(vfsnode_t *, const char *, int *);
  int (*opendir)(vfsnode_t *, vfs_dir_t *, const char *);
  vfs_dirent_t *(*readdir)(vfsnode_t *, vfs_dir_t *);
  void (*closedir)(vfsnode_t *, vfs_dir_t *);
  int (*stat)(vfsnode_t *, const char *, vfs_stat_t *);
};

struct vfs_dir_s
{
  vfs_dir_t *link;
  vfsnode_t *node;
  vfs_dirent_t *dirent;
  void *posp;
};

vfsnode_t *vfsnode_mknode(vfsnode_t *parent, const char *name, vfsnode_vtable_t *vtable, void *context);
vfsnode_t *vfsnode_mkvirtnode(vfsnode_t *parent, const char *name);

vfsnode_t *vfsnode_find(const char *path, int *offs);

vfs_dirent_t *vfsnode_readdir(vfs_dir_t *dir);
vfs_dir_t *vfsnode_opendir(vfsnode_t *node, const char *path);
int vfsnode_closedir(vfs_dir_t *dir);

int vfsnode_stat(vfsnode_t *node, const char *path, vfs_stat_t *st);


void vfsnode_init(void);

#endif				/* __VFSNODE_H__ */