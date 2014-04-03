//---------------------------------------------------------------------------
//  __________________    _________  _____            _____  .__         ._.
//  \______   \______ \  /   _____/ /     \          /  _  \ |__| ____   | |
//   |    |  _/|    |  \ \_____  \ /  \ /  \        /  /_\  \|  _/ __ \  | |
//   |    |   \|    `   \/        /    Y    \      /    |    |  \  ___/   \|
//   |______  /_______  /_______  \____|__  / /\   \____|__  |__|\___ |   __
//          \/        \/        \/        \/  )/           \/        \/   \/
//
// This file is part of libdsm. Copyright © 2014 VideoLabs SAS
//
// Author: Julien 'Lta' BALLET <contact@lta.io>
//
// This program is free software. It comes without any warranty, to the extent
// permitted by applicable law. You can redistribute it and/or modify it under
// the terms of the Do What The Fuck You Want To Public License, Version 2, as
// published by Sam Hocevar. See the COPYING file for more details.
//----------------------------------------------------------------------------

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "bdsm/smb_message.h"
#include "bdsm/smb_utils.h"

smb_message_t   *smb_message_new(uint8_t cmd, size_t payload_size)
{
  const char    magic[4] = SMB_MAGIC;
  smb_message_t *msg;

  msg = (smb_message_t *)malloc(sizeof(smb_message_t));
  assert(msg != NULL);
  memset((void *)msg, 0, sizeof(smb_message_t));

  msg->packet = (smb_packet_t *)malloc(sizeof(smb_message_t) + payload_size);
  assert(msg != NULL);
  memset((void *)msg->packet, 0, sizeof(smb_message_t) + payload_size);

  msg->payload_size = payload_size;
  msg->cursor = 0;

  for (unsigned i = 0; i < 4; i++)
    msg->packet->header.magic[i] = magic[i];
  msg->packet->header.command   = cmd;
  msg->packet->header.pid       = getpid();

  return (msg);
}

void            smb_message_destroy(smb_message_t *msg)
{
  if (msg != NULL)
  {
    if (msg->packet != NULL)
      free(msg->packet);
    free(msg);
  }
}

int             smb_message_append(smb_message_t *msg, const void *data,
                                   size_t data_size)
{
  assert(msg != NULL && data != NULL);

  if (data_size > msg->payload_size - msg->cursor)
    return (0);

  memcpy(msg->packet->payload + msg->cursor, data, data_size);
  msg->cursor += data_size;

  return (1);
}

int             smb_message_advance(smb_message_t *msg, size_t size)
{
  assert(msg != NULL);

  if (msg->cursor + size > msg->payload_size)
    return (0);

  msg->cursor += size;
  return (1);
}

int             smb_message_put8(smb_message_t *msg, uint8_t data)
{
  return(smb_message_append(msg, (void *)&data, 1));
}

int             smb_message_put16(smb_message_t *msg, uint16_t data)
{
    return(smb_message_append(msg, (void *)&data, 2));
}

int             smb_message_put32(smb_message_t *msg, uint32_t data)
{
    return(smb_message_append(msg, (void *)&data, 4));
}

int             smb_message_put64(smb_message_t *msg, uint32_t data)
{
    return(smb_message_append(msg, (void *)&data, 8));
}

size_t          smb_message_put_utf16(smb_message_t *msg, const char *src_enc,
                                      const char *str, size_t str_len)
{
  char          *utf_str;
  size_t        utf_str_len;
  int           res;

  utf_str_len = smb_to_utf16(src_enc, str, str_len, &utf_str);
  res = smb_message_append(msg, utf_str, utf_str_len);
  free(utf_str);

  if (res)
    return(utf_str_len);
  return (0);
}

void            smb_message_flag(smb_message_t *msg, uint32_t flag, int value)
{
  uint32_t      *flags;

  assert(msg != NULL && msg->packet != NULL);

  // flags + flags2 is actually 24 bit long, we have to be cautious
  flags = (uint32_t *)&(msg->packet->header.flags);
  flag &= 0x00FFFFFF;

  if (value)
    *flags |= flag;
  else
    *flags &= ~flag;
}

void            smb_message_set_default_flags(smb_message_t *msg)
{
  assert(msg != NULL && msg->packet != NULL);

  msg->packet->header.flags   = 0x18;
  //msg->packet->header.flags2  = 0xc843; // w/ extended security;
  msg->packet->header.flags2  = 0xc043; // w/o extended security;
}

void            smb_message_set_andx_members(smb_message_t *msg)
{
  // This could have been any type with the 'SMB_ANDX_MEMBERS';
  smb_session_req_t   *req;

  assert(msg != NULL);

  req = (smb_session_req_t *)msg->packet->payload;
  req->andx           = 0xff;
  req->andx_reserved  = 0;
  req->andx_offset    = 0;
}
