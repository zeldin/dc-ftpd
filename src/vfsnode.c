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

static vfsnode_t *rootnode = NULL;

typedef struct virtnode_private_s {
  vfsnode_t *first_child, *last_child;
} virtnode_private_t;

static void virtnode_init(vfsnode_t *node, void *context)
{
  virtnode_private_t *private = calloc(1, sizeof(virtnode_private_t));
  if (private) {
    private->first_child = NULL;
    private->last_child = NULL;
    node->private = private;
  }
}

static void virtnode_destroy(vfsnode_t *node)
{
  virtnode_private_t *private = (virtnode_private_t *)node->private;
  if (private) {
    vfsnode_t *child;
    while ((child = private->first_child))
      vfsnode_destroy(child);
  }
}

static void virtnode_add_child(vfsnode_t *node, vfsnode_t *childnode)
{
  virtnode_private_t *private = (virtnode_private_t *)node->private;
  if (private) {
    childnode->sibling = NULL;
    if (private->last_child != NULL) {
      private->last_child->sibling = childnode;
    } else {
      private->first_child = childnode;
    }
    private->last_child = childnode;
  }
}

static void virtnode_remove_child(vfsnode_t *node, vfsnode_t *childnode)
{
  virtnode_private_t *private = (virtnode_private_t *)node->private;
  if (private)
    if (childnode == private->first_child) {
      if ((private->first_child = childnode->sibling) == NULL)
	private->last_child = NULL;
    } else {
      vfsnode_t *pre;
      for (pre = private->first_child; pre; pre = pre->sibling)
	if (pre->sibling == childnode) {
	  if ((pre->sibling = childnode->sibling) == NULL)
	    private->last_child = pre;
	  break;
	}
    }
}

static int compare_name(const char *a, const char *b)
{
  int c, n=0;
  while(*a++ == (c=*b++))
    if(!c)
      return n;
    else
      n++;
  if(c)
    return 0;
  return ((*--a == '/')? n : 0);
}

static vfsnode_t *virtnode_find(vfsnode_t *node, const char *path, int *offs)
{
  virtnode_private_t *private = (virtnode_private_t *)node->private;
  if (private) {
    int o = 0;
    while(path[o] == '/')
      o++;
    if (path[o]) {
      int z;
      for(node = private->first_child; node != NULL; node = node->sibling)
	if ((z = compare_name(path+o, node->name)))
	  break;
      if (node) {
	o += z;
	while(path[o] == '/' && path[o+1] == '/')
	  o++;
      }
    }
    *offs = o;
    return node;
  } else
    return NULL;
}

static int virtnode_opendir(vfsnode_t *node, vfs_dir_t *dir, const char *path)
{
  virtnode_private_t *private = (virtnode_private_t *)node->private;
  if (*path || !private) return -ENOTDIR;
  dir->posp = private->first_child;
  return 0;
}

static vfs_dirent_t *virtnode_readdir(vfsnode_t *node_, vfs_dir_t *dir)
{
  vfsnode_t *node = dir->posp;
  if (node) {
    vfs_dirent_t *de = calloc(1, sizeof(vfs_dirent_t)+1+strlen(node->name));
    if (de) {
      dir->posp = node->sibling;
      strcpy(de->name, node->name);
    } else
      dir->posp = NULL;
    return de;
  } else
    return NULL;
}

static int virtnode_stat(vfsnode_t *node, const char *path, vfs_stat_t *st)
{
  st->st_mode = 1;
  return 0;
}

static vfsnode_vtable_t virtnode_vtable = {
  .init = virtnode_init,
  .destroy = virtnode_destroy,
  .add_child = virtnode_add_child,
  .remove_child = virtnode_remove_child,
  .find = virtnode_find,
  .opendir = virtnode_opendir,
  .readdir = virtnode_readdir,
  .stat = virtnode_stat,
};

typedef struct rom_s { const void *data; size_t len; } rom_t;

typedef struct romnode_private_s {
  rom_t rom;
} romnode_private_t;

static void romnode_init(vfsnode_t *node, void *context)
{
  romnode_private_t *private = calloc(1, sizeof(romnode_private_t));
  if (private) {
    private->rom = *(const rom_t *)context;
    node->private = private;
  }
}

static int romnode_stat(vfsnode_t *node, const char *path, vfs_stat_t *st)
{
  romnode_private_t *private = (romnode_private_t *)node->private;
  if (private) {
    st->st_size = private->rom.len;
  } else
    return -ENOENT;
}

static int romnode_open(vfsnode_t *node, vfs_file_t *file, const char *path,
			int write_mode)
{
  if (*path)
    return -ENOENT;
  if (write_mode)
    return -EROFS;
  file->posn = 0;
  return 0;
}

static int romnode_read(vfsnode_t *node, vfs_file_t *file, void *buffer,
			size_t size, size_t nmemb)
{
  romnode_private_t *private = (romnode_private_t *)node->private;
  if (private) {
    size_t bytes, cnt = (private->rom.len - file->posn)/size;
    if (cnt > nmemb)
      cnt = nmemb;
    bytes = cnt * size;
    if (bytes) {
      memcpy(buffer, ((const char *)private->rom.data) + file->posn, bytes);
      file->posn += bytes;
    }
    return cnt;
  } else
    return 0;
}

static vfsnode_vtable_t romnode_vtable = {
  .init = romnode_init,
  .stat = romnode_stat,
  .open = romnode_open,
  .read = romnode_read,
};


vfsnode_t *vfsnode_mknode(vfsnode_t *parent, const char *name, vfsnode_vtable_t *vtable, void *context)
{
  vfsnode_t *node = calloc(1, sizeof(vfsnode_t)+1+strlen(name));
  if (node) {
    node->vtable = vtable;
    if (parent == NULL)
      parent = rootnode;
    node->parent = parent;
    node->root = rootnode;
    node->sibling = NULL;
    node->private = NULL;
    node->dirs = NULL;
    node->files = NULL;
    strcpy(node->name, name);
    if (vtable->init)
      vtable->init(node, context);
    if (parent != NULL && parent->vtable->add_child)
      parent->vtable->add_child(parent, node);
  }
  return node;
}

vfsnode_t *vfsnode_mkvirtnode(vfsnode_t *parent, const char *name)
{
  return vfsnode_mknode(parent, name, &virtnode_vtable, NULL);
}

vfsnode_t *vfsnode_mkromnode(vfsnode_t *parent, const char *name,
			     const void *data, size_t len)
{
  rom_t rom = { .data = data, .len = len };
  return vfsnode_mknode(parent, name, &romnode_vtable, &rom);
}

void vfsnode_destroy(vfsnode_t *node)
{
  if (node == rootnode)
    rootnode = NULL;
  if (node->parent) {
    if (node->parent->vtable->remove_child)
      node->parent->vtable->remove_child(node->parent, node);
    node->parent = NULL;
  }
  while (node->dirs) {
    vfs_dir_t *dd = node->dirs;
    node->dirs = dd->link;
    dd->link = NULL;
    if (dd->node == node &&
	node->vtable->closedir)
      node->vtable->closedir(node, dd);
    dd->node = NULL;
  }
  while (node->files) {
    vfs_file_t *ff = node->files;
    node->files = ff->link;
    ff->link = NULL;
    if (ff->node == node &&
	node->vtable->close)
      node->vtable->close(node, ff);
    ff->node = NULL;
  }
  if (node->vtable->destroy)
    node->vtable->destroy(node);
  if (node->private)
    free(node->private);
  free(node);
}

vfsnode_t *vfsnode_find(const char *path, int *offs)
{
  vfsnode_t *node = rootnode;
  int loffs, toffs = 0;
  while (node && node->vtable->find) {
    node = rootnode->vtable->find(node, path, &loffs);
    if (!node || !loffs)
      break;
    toffs += loffs;
    path += loffs;
  }
  if (node && offs)
    *offs = toffs;
  return node;
}

vfs_dirent_t *vfsnode_readdir(vfs_dir_t *dir)
{
  if (dir && dir->node && dir->node->vtable->readdir) {
    if (dir->dirent)
      free(dir->dirent);
    return (dir->dirent = dir->node->vtable->readdir(dir->node, dir));
  } else
    return NULL;
}

vfs_dir_t *vfsnode_opendir(vfsnode_t *node, const char *path)
{
  if (node->vtable->opendir) {
    vfs_dir_t *dir = calloc(1, sizeof(vfs_dir_t));
    dir->link = NULL;
    dir->node = node;
    dir->dirent = NULL;
    if (node->vtable->opendir(node, dir, path)) {
      free(dir);
      dir = NULL;
    } else {
      dir->link = node->dirs;
      node->dirs = dir;
    }
    return dir;
  } else
    return NULL;
}

int vfsnode_closedir(vfs_dir_t *dir)
{
  vfsnode_t *node;
  if(!dir)
    return -EBADF;
  node = dir->node;
  if (node) {
    if (node->vtable->closedir)
      node->vtable->closedir(node, dir);
    if (node->dirs == dir)
      node->dirs = dir->link;
    else {
      vfs_dir_t *dd;
      for(dd = node->dirs; dd; dd = dd->link)
	if (dd->link == dir) {
	  dd->link = dir->link;
	  break;
	}
    }
  }
  if (dir->dirent)
    free(dir->dirent);
  free(dir);
  return 0;
}

int vfsnode_stat(vfsnode_t *node, const char *path, vfs_stat_t *st)
{
  if (node->vtable->stat) {
    memset(st, 0, sizeof(vfs_stat_t));
    return node->vtable->stat(node, path, st);
  } else
    return -ENOSYS;
}

vfs_file_t *vfsnode_open(vfsnode_t *node, const char *path, int write_mode)
{
  if (node->vtable->open) {
    vfs_file_t *file = calloc(1, sizeof(vfs_file_t));
    file->link = NULL;
    file->node = node;
    file->eof = 0;
    if (node->vtable->open(node, file, path, write_mode)) {
      free(file);
      file = NULL;
    } else {
      file->link = node->files;
      node->files = file;
    }
    return file;
  } else
    return NULL;
}

int vfsnode_read(void *buffer, size_t size, size_t nmemb, vfs_file_t *file)
{
  vfsnode_t *node;
  if(!file)
    return -EBADF;
  node = file->node;
  if (node) {
    int r = 0;
    if (node->vtable->read)
      r = node->vtable->read(node, file, buffer, size, nmemb);
    if (r >= 0 && r < nmemb)
      file->eof = 1;
    return r;
  } else
    return 0;
}

int vfsnode_eof(vfs_file_t *file)
{
  vfsnode_t *node;
  if(!file)
    return -EBADF;
  node = file->node;
  if (node) {
    if (node->vtable->eof)
      return node->vtable->eof(node, file);
    else
      return file->eof;
  } else
    return 1;
}

int vfsnode_close(vfs_file_t *file)
{
  vfsnode_t *node;
  if(!file)
    return -EBADF;
  node = file->node;
  if (node) {
    if (node->vtable->close)
      node->vtable->close(node, file);
    if (node->files == file)
      node->files = file->link;
    else {
      vfs_file_t *ff;
      for(ff = node->files; ff; ff = ff->link)
	if (ff->link == file) {
	  ff->link = file->link;
	  break;
	}
    }
  }
  free(file);
  return 0;
}

void vfsnode_init(void)
{
  rootnode = vfsnode_mkvirtnode(NULL, "");
}
