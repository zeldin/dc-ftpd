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

static void make_vfsnodes_session(vfsnode_t *parent, int n)
{
  if (!parent)
    return;

  vfsnode_mkromnode(parent, "toc", &toc[n], sizeof(toc[n]));
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
  
  for(i=0; i<2; i++) {
    struct { int session; void *buffer; } param;
    param.session = i;
    param.buffer = &toc[i];
    memset(&toc[i], 0, sizeof(toc[i]));
    tocr[i] = exec_cmd(19, &param);
  }

  if (tocr[0]<0 && tocr[1]<0)
    return;

  vfs_lock();
  root = vfsnode_mkvirtnode(NULL, "gdrom");
  if (root != NULL)
    for(i=0; i<2; i++) {
      char name[16];
      sprintf(name, "session%d", i+1);
      if (tocr[i]>=0)
	make_vfsnodes_session(vfsnode_mkvirtnode(root, name), i);
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
