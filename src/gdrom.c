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
#include <lwip/sys.h>
#include <ronin/gddrive.h>
#include <ronin/cdfs.h>

#include "vfs.h"
#include "vfsnode.h"
#include "backends.h"

static sys_mbox_t mbox;

#define CHK_STATUS_INTERVAL   500 /* twice per second */

static vfsnode_t *root = NULL;
static struct TOC toc[2];
static int curr_secsize, curr_secmode;

static int gdfs_errno_to_errno(int n)
{
 switch(n) {
   case 2:
     return -ENOMEDIUM;
   case 6:
     return -ESTALE;
   default:
     return -EIO;
  }
}

static int getCdState()
{
  unsigned int param[4];
  gdGdcGetDrvStat(param);
  return param[0];
}

static int send_cmd(int cmd, void *param)
{
  return gdGdcReqCmd(cmd, param);
}

static int check_cmd(int f)
{
  int blah[4];
  int n;
  gdGdcExecServer();
  if((n = gdGdcGetCmdStat(f, blah))==1)
    return 0;
  if(n == 2)
    return 1;
  else return gdfs_errno_to_errno(blah[0]);
}

static int wait_cmd(int f)
{
  int n;
  while(!(n = check_cmd(f)));
  return (n>0? 0 : n);
}

static int exec_cmd(int cmd, void *param)
{
  int f = send_cmd(cmd, param);
  return wait_cmd(f);
}

static int read_sectors(int sec, int secsize, int secmode, char *buf, int num)
{
  struct { int sec, num; void *buffer; int dunno; } param;
  if (secsize != curr_secsize || secmode != curr_secmode) {
    unsigned int param[4];
    param[0] = 0; /* set data type */
    param[1] = 8192;
    param[2] = secmode;
    param[3] = secsize;
    curr_secsize = curr_secmode = -1;
    if(gdGdcChangeDataType(param)<0)
      return -EIO;
    curr_secsize = secsize;
    curr_secmode = secmode;
  }
  param.sec = sec;
  param.num = num;
  param.buffer = buf;
  param.dunno = 0;
  return exec_cmd(16, &param);
}

typedef struct gdrom_track_s {
  int start, end, sectorsize, sectormode;
  unsigned char ctrl, adr;
} gdrom_track_t;

typedef struct tracknode_private_s {
  gdrom_track_t track;
} tracknode_private_t;

static void tracknode_init(vfsnode_t *node, void *context)
{
  tracknode_private_t *private = calloc(1, sizeof(tracknode_private_t));
  if (private) {
    private->track = *(gdrom_track_t *)context;
    node->private = private;
  }
}

static int tracknode_stat(vfsnode_t *node, const char *path, vfs_stat_t *st)
{
  tracknode_private_t *private = (tracknode_private_t *)node->private;
  if (private) {
    st->st_size = private->track.sectorsize *
      (private->track.end - private->track.start);
  } else
    return -ENOENT;
}

static int tracknode_open(vfsnode_t *node, vfs_file_t *file, const char *path,
			  int write_mode)
{
  if (*path)
    return -ENOENT;
  if (write_mode)
    return -EROFS;
  file->posn = 0;
  return 0;
}

static int tracknode_read(vfsnode_t *node, vfs_file_t *file, void *buffer,
			  size_t size, size_t nmemb)
{
  tracknode_private_t *private = (tracknode_private_t *)node->private;
  if (private) {
    size_t bytes, cnt =
      (private->track.sectorsize * (private->track.end - private->track.start)
       - file->posn)/size;
    if (cnt > nmemb)
      cnt = nmemb;
    bytes = cnt * size;
    if (bytes) {
      int sec = file->posn / private->track.sectorsize + private->track.start;
      int offs = file->posn % private->track.sectorsize;
      int bl = bytes;
      char buf[2352];
      if (offs || bl < private->track.sectorsize) {
	int r = read_sectors(sec, private->track.sectorsize,
			     private->track.sectormode, buf, 1);
	if (r<0)
	  return r;
	sec++;
	if (offs + bl > private->track.sectorsize) {
	  memcpy(buffer, buf+offs, private->track.sectorsize-offs);
	  buffer = ((char *)buffer)+private->track.sectorsize-offs;
	  bl -= private->track.sectorsize-offs;
	} else {
	  memcpy(buffer, buf+offs, bl);
	  buffer = ((char *)buffer)+bl;
	  bl = 0;
	}
      }
      if (bl >= private->track.sectorsize) {
	int sn = bl/private->track.sectorsize;
	int r = read_sectors(sec, private->track.sectorsize,
			     private->track.sectormode, buffer, sn);
	if (r<0)
	  return r;
	sec += sn;
	buffer = ((char *)buffer)+bl;
	bl %= private->track.sectorsize;
	buffer = ((char *)buffer)-bl;
      }
      if (bl) {
	int r = read_sectors(sec, private->track.sectorsize,
			     private->track.sectormode, buf, 1);
	if (r<0)
	  return r;
	memcpy(buffer, buf, bl);
      }
      file->posn += bytes;
    }
    return cnt;
  } else
    return 0;
}

static vfsnode_vtable_t tracknode_vtable = {
  .init = tracknode_init,
  .stat = tracknode_stat,
  .open = tracknode_open,
  .read = tracknode_read,
};

static void make_vfsnodes_track(vfsnode_t *parent, int n, int t, int *param,
				unsigned int entry, unsigned int next)
{
  char name[16];
  int datatrack = TOC_CTRL(entry)&4;
  int cdxa = (param[1] == 32);
  gdrom_track_t track = { .start = TOC_LBA(entry),
			  .end = TOC_LBA(next),
			  .ctrl = TOC_CTRL(entry),
			  .adr = TOC_ADR(entry),
			  .sectorsize = (datatrack? 2048 : 2352),
			  .sectormode = (datatrack? (cdxa? 2048 : 1024) : 0) };
  sprintf(name, "track%02d.%s", t, (datatrack? "iso" : "cdda"));
  if (track.end >= track.start)
    vfsnode_mknode(parent, name, &tracknode_vtable, &track);
}

static void make_vfsnodes_session(vfsnode_t *parent, int n, int *param)
{
  int track;

  if (!parent)
    return;

  vfsnode_mkromnode(parent, "toc", &toc[n], sizeof(toc[n]));
  for(track = TOC_TRACK(toc[n].first); track <= TOC_TRACK(toc[n].last);
      track++)
    if (track >= 1 && track <= 99)
      make_vfsnodes_track(parent, n, track, param, toc[n].entry[track-1],
			  toc[n].entry[(track == TOC_TRACK(toc[n].last)?
					101 : track)]);
}

static void make_vfsnodes(void)
{
  int i, r=0;
  unsigned int param[4];
  int tocr[2];

  for(i=0; i<8; i++)
    if(!(r = exec_cmd(24, NULL)))
      break;
  if(r)
    return;

  curr_secsize = curr_secmode = -1;
  
  for(i=0; i<2; i++) {
    struct { int session; void *buffer; } param;
    param.session = i;
    param.buffer = &toc[i];
    memset(&toc[i], 0, sizeof(toc[i]));
    tocr[i] = exec_cmd(19, &param);
  }

  if (tocr[0]<0 && tocr[1]<0)
    return;

  gdGdcGetDrvStat(param);

  vfs_lock();
  root = vfsnode_mkvirtnode(NULL, "gdrom");
  if (root != NULL)
    for(i=0; i<2; i++) {
      char name[16];
      sprintf(name, "session%d", i+1);
      if (tocr[i]>=0)
	make_vfsnodes_session(vfsnode_mkvirtnode(root, name), i, param);
    }
  vfs_unlock();
}

static void chk_drivestatus(void *arg)
{
  static int oldstate = -1;
  int newstate = getCdState();
  if (newstate != oldstate) {
    sys_mbox_post(mbox, (void *)newstate);
    oldstate = newstate;
  }
  sys_timeout(CHK_STATUS_INTERVAL, (sys_timeout_handler)chk_drivestatus, NULL);
}

static void gdrom_thread(void *arg)
{
  void *msg;
  int state;

  sys_timeout(CHK_STATUS_INTERVAL, (sys_timeout_handler)chk_drivestatus, NULL);

  for(;;) {
    sys_mbox_fetch(mbox, &msg);
    state = (int)msg;
    if (state > 0 && state < 6) {
      if (root == NULL)
	make_vfsnodes();
    } else if(root) {
      vfs_lock();
      vfsnode_destroy(root);
      root = NULL;
      vfs_unlock();
    }
  }
}

void gdrom_be_init(void)
{
  cdfs_init();
  mbox = sys_mbox_new();
  sys_thread_new((void *)gdrom_thread, NULL);
}
