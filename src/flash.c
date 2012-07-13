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

#include <errno.h>

#include "vfs.h"
#include "vfsnode.h"
#include "backends.h"

static int syscall_info_flash(int sect, int *info)
{
  return (*(int (**)())0x8c0000b8)(sect,info,0,0);  
}

static int syscall_read_flash(int offs, void *buf, int cnt)
{
  return (*(int (**)())0x8c0000b8)(offs,buf,cnt,1);
}

typedef struct flashnode_private_s {
  int offs, len;
} flashnode_private_t;

static void flashnode_init(vfsnode_t *node, void *context)
{
  flashnode_private_t *private = calloc(1, sizeof(flashnode_private_t));
  if (private) {
    private->offs = ((const int *)context)[0];
    private->len = ((const int *)context)[1];
    node->private = private;
  }
}

static int flashnode_stat(vfsnode_t *node, const char *path, vfs_stat_t *st)
{
  flashnode_private_t *private = (flashnode_private_t *)node->private;
  if (private) {
    st->st_size = private->len;
  } else
    return -ENOENT;
}

static int flashnode_open(vfsnode_t *node, vfs_file_t *file, const char *path,
			int write_mode)
{
  if (*path)
    return -ENOENT;
  if (write_mode)
    return -EROFS;
  file->posn = 0;
  return 0;
}

static int flashnode_read(vfsnode_t *node, vfs_file_t *file, void *buffer,
			size_t size, size_t nmemb)
{
  flashnode_private_t *private = (flashnode_private_t *)node->private;
  if (private) {
    size_t bytes, cnt = (private->len - file->posn)/size;
    if (cnt > nmemb)
      cnt = nmemb;
    bytes = cnt * size;
    if (bytes) {
      int r = syscall_read_flash(private->offs + file->posn, buffer, bytes);
      if (r<0)
	return -EIO;
      file->posn += bytes;
    }
    return cnt;
  } else
    return 0;
}

static vfsnode_vtable_t flashnode_vtable = {
  .init = flashnode_init,
  .stat = flashnode_stat,
  .open = flashnode_open,
  .read = flashnode_read,
};

void flash_be_init(void)
{
  vfsnode_t *root = vfsnode_mkvirtnode(NULL, "flash");
  if (root) {
    int p;
    int info[2];
    for(p=0; p<16; p++)
      if(!syscall_info_flash(p, info)) {
	char buf[16];
	sprintf(buf, "partition%d", p);
	vfsnode_mknode(root, buf, &flashnode_vtable, info);
      }
  }
  vfsnode_mkromnode(NULL, "rom", (const void *)0xa0000000, 2*1024*1024);
}
