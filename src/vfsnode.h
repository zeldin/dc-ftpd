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

struct vfsnode_s {
  vfsnode_vtable_t *vtable;
  vfsnode_t *parent, *root, *sibling;
  vfs_dir_t *dirs;
  vfs_file_t *files;
  void *private;
  char name[];
};

struct vfsnode_vtable_s
{
  void (*init)(vfsnode_t *, void *);
  void (*destroy)(vfsnode_t *);
  void (*add_child)(vfsnode_t *, vfsnode_t *);
  void (*remove_child)(vfsnode_t *, vfsnode_t *);
  vfsnode_t *(*find)(vfsnode_t *, const char *, int *);
  int (*opendir)(vfsnode_t *, vfs_dir_t *, const char *);
  vfs_dirent_t *(*readdir)(vfsnode_t *, vfs_dir_t *);
  void (*closedir)(vfsnode_t *, vfs_dir_t *);
  int (*stat)(vfsnode_t *, const char *, vfs_stat_t *);
  int (*open)(vfsnode_t *, vfs_file_t *, const char *, int);
  int (*read)(vfsnode_t *, vfs_file_t *, void *, size_t, size_t);
  int (*eof)(vfsnode_t *, vfs_file_t *);
  int (*close)(vfsnode_t *, vfs_file_t *);
};

struct vfs_dir_s
{
  vfs_dir_t *link;
  vfsnode_t *node;
  vfs_dirent_t *dirent;
  void *posp;
  unsigned long posn;
};

struct vfs_file_s
{
  vfs_file_t *link;
  vfsnode_t *node;
  int eof;
  void *posp;
  unsigned long posn;
};

vfsnode_t *vfsnode_mknode(vfsnode_t *parent, const char *name, vfsnode_vtable_t *vtable, void *context);
vfsnode_t *vfsnode_mkvirtnode(vfsnode_t *parent, const char *name);
vfsnode_t *vfsnode_mkromnode(vfsnode_t *parent, const char *name,
			     const void *data, size_t len);
void vfsnode_destroy(vfsnode_t *node);

vfsnode_t *vfsnode_find(const char *path, int *offs);

vfs_dirent_t *vfsnode_readdir(vfs_dir_t *dir);
vfs_dir_t *vfsnode_opendir(vfsnode_t *node, const char *path);
int vfsnode_closedir(vfs_dir_t *dir);

int vfsnode_stat(vfsnode_t *node, const char *path, vfs_stat_t *st);

vfs_file_t *vfsnode_open(vfsnode_t *node, const char *path, int write_mode);
int vfsnode_read(void *buffer, size_t size, size_t nmemb, vfs_file_t *file);
int vfsnode_eof(vfs_file_t *file);
int vfsnode_close(vfs_file_t *file);


void vfsnode_init(void);

#endif				/* __VFSNODE_H__ */
