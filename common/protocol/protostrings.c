/* Generated from nbd-protocol.h by generate-protostrings.sh.
 * License of this file is BSD, the same as the rest of nbdkit.
 */

#include <stdio.h>
#include "nbd-protocol.h"

extern const char *
name_of_nbd_cmd (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_CMD_READ:
    return "NBD_CMD_READ";
  case NBD_CMD_WRITE:
    return "NBD_CMD_WRITE";
  case NBD_CMD_DISC:
    return "NBD_CMD_DISC";
  case NBD_CMD_FLUSH:
    return "NBD_CMD_FLUSH";
  case NBD_CMD_TRIM:
    return "NBD_CMD_TRIM";
  case NBD_CMD_CACHE:
    return "NBD_CMD_CACHE";
  case NBD_CMD_WRITE_ZEROES:
    return "NBD_CMD_WRITE_ZEROES";
  case NBD_CMD_BLOCK_STATUS:
    return "NBD_CMD_BLOCK_STATUS";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

extern const char *
name_of_nbd_cmd_flag (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_CMD_FLAG_FUA:
    return "NBD_CMD_FLAG_FUA";
  case NBD_CMD_FLAG_NO_HOLE:
    return "NBD_CMD_FLAG_NO_HOLE";
  case NBD_CMD_FLAG_DF:
    return "NBD_CMD_FLAG_DF";
  case NBD_CMD_FLAG_REQ_ONE:
    return "NBD_CMD_FLAG_REQ_ONE";
  case NBD_CMD_FLAG_FAST_ZERO:
    return "NBD_CMD_FLAG_FAST_ZERO";
  case NBD_CMD_FLAG_PAYLOAD_LEN:
    return "NBD_CMD_FLAG_PAYLOAD_LEN";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

extern const char *
name_of_nbd_error (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_SUCCESS:
    return "NBD_SUCCESS";
  case NBD_EPERM:
    return "NBD_EPERM";
  case NBD_EIO:
    return "NBD_EIO";
  case NBD_ENOMEM:
    return "NBD_ENOMEM";
  case NBD_EINVAL:
    return "NBD_EINVAL";
  case NBD_ENOSPC:
    return "NBD_ENOSPC";
  case NBD_EOVERFLOW:
    return "NBD_EOVERFLOW";
  case NBD_ENOTSUP:
    return "NBD_ENOTSUP";
  case NBD_ESHUTDOWN:
    return "NBD_ESHUTDOWN";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

extern const char *
name_of_nbd_flag (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_FLAG_HAS_FLAGS:
    return "NBD_FLAG_HAS_FLAGS";
  case NBD_FLAG_READ_ONLY:
    return "NBD_FLAG_READ_ONLY";
  case NBD_FLAG_SEND_FLUSH:
    return "NBD_FLAG_SEND_FLUSH";
  case NBD_FLAG_SEND_FUA:
    return "NBD_FLAG_SEND_FUA";
  case NBD_FLAG_ROTATIONAL:
    return "NBD_FLAG_ROTATIONAL";
  case NBD_FLAG_SEND_TRIM:
    return "NBD_FLAG_SEND_TRIM";
  case NBD_FLAG_SEND_WRITE_ZEROES:
    return "NBD_FLAG_SEND_WRITE_ZEROES";
  case NBD_FLAG_SEND_DF:
    return "NBD_FLAG_SEND_DF";
  case NBD_FLAG_CAN_MULTI_CONN:
    return "NBD_FLAG_CAN_MULTI_CONN";
  case NBD_FLAG_SEND_CACHE:
    return "NBD_FLAG_SEND_CACHE";
  case NBD_FLAG_SEND_FAST_ZERO:
    return "NBD_FLAG_SEND_FAST_ZERO";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

extern const char *
name_of_nbd_global_flag (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_FLAG_FIXED_NEWSTYLE:
    return "NBD_FLAG_FIXED_NEWSTYLE";
  case NBD_FLAG_NO_ZEROES:
    return "NBD_FLAG_NO_ZEROES";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

extern const char *
name_of_nbd_info (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_INFO_EXPORT:
    return "NBD_INFO_EXPORT";
  case NBD_INFO_NAME:
    return "NBD_INFO_NAME";
  case NBD_INFO_DESCRIPTION:
    return "NBD_INFO_DESCRIPTION";
  case NBD_INFO_BLOCK_SIZE:
    return "NBD_INFO_BLOCK_SIZE";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

extern const char *
name_of_nbd_opt (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_OPT_EXPORT_NAME:
    return "NBD_OPT_EXPORT_NAME";
  case NBD_OPT_ABORT:
    return "NBD_OPT_ABORT";
  case NBD_OPT_LIST:
    return "NBD_OPT_LIST";
  case NBD_OPT_STARTTLS:
    return "NBD_OPT_STARTTLS";
  case NBD_OPT_INFO:
    return "NBD_OPT_INFO";
  case NBD_OPT_GO:
    return "NBD_OPT_GO";
  case NBD_OPT_STRUCTURED_REPLY:
    return "NBD_OPT_STRUCTURED_REPLY";
  case NBD_OPT_LIST_META_CONTEXT:
    return "NBD_OPT_LIST_META_CONTEXT";
  case NBD_OPT_SET_META_CONTEXT:
    return "NBD_OPT_SET_META_CONTEXT";
  case NBD_OPT_EXTENDED_HEADERS:
    return "NBD_OPT_EXTENDED_HEADERS";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

extern const char *
name_of_nbd_rep (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_REP_ACK:
    return "NBD_REP_ACK";
  case NBD_REP_SERVER:
    return "NBD_REP_SERVER";
  case NBD_REP_INFO:
    return "NBD_REP_INFO";
  case NBD_REP_META_CONTEXT:
    return "NBD_REP_META_CONTEXT";
  case NBD_REP_ERR_UNSUP:
    return "NBD_REP_ERR_UNSUP";
  case NBD_REP_ERR_POLICY:
    return "NBD_REP_ERR_POLICY";
  case NBD_REP_ERR_INVALID:
    return "NBD_REP_ERR_INVALID";
  case NBD_REP_ERR_PLATFORM:
    return "NBD_REP_ERR_PLATFORM";
  case NBD_REP_ERR_TLS_REQD:
    return "NBD_REP_ERR_TLS_REQD";
  case NBD_REP_ERR_UNKNOWN:
    return "NBD_REP_ERR_UNKNOWN";
  case NBD_REP_ERR_SHUTDOWN:
    return "NBD_REP_ERR_SHUTDOWN";
  case NBD_REP_ERR_BLOCK_SIZE_REQD:
    return "NBD_REP_ERR_BLOCK_SIZE_REQD";
  case NBD_REP_ERR_TOO_BIG:
    return "NBD_REP_ERR_TOO_BIG";
  case NBD_REP_ERR_EXT_HEADER_REQD:
    return "NBD_REP_ERR_EXT_HEADER_REQD";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

extern const char *
name_of_nbd_reply (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_REPLY_FLAG_DONE:
    return "NBD_REPLY_FLAG_DONE";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

extern const char *
name_of_nbd_reply_type (unsigned int fl)
{
  static char buf[] = "unknown (0x00000000)";
  switch (fl) {
  case NBD_REPLY_TYPE_NONE:
    return "NBD_REPLY_TYPE_NONE";
  case NBD_REPLY_TYPE_OFFSET_DATA:
    return "NBD_REPLY_TYPE_OFFSET_DATA";
  case NBD_REPLY_TYPE_OFFSET_HOLE:
    return "NBD_REPLY_TYPE_OFFSET_HOLE";
  case NBD_REPLY_TYPE_BLOCK_STATUS:
    return "NBD_REPLY_TYPE_BLOCK_STATUS";
  case NBD_REPLY_TYPE_BLOCK_STATUS_EXT:
    return "NBD_REPLY_TYPE_BLOCK_STATUS_EXT";
  case NBD_REPLY_TYPE_ERROR:
    return "NBD_REPLY_TYPE_ERROR";
  case NBD_REPLY_TYPE_ERROR_OFFSET:
    return "NBD_REPLY_TYPE_ERROR_OFFSET";

  default:
    snprintf (buf, sizeof buf, "unknown (0x%x)", fl);
    return buf;
  }
}

