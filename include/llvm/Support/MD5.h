/*
 * This code is derived from (original license follows):
 *
 * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
 * MD5 Message-Digest Algorithm (RFC 1321).
 *
 * Homepage:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * See md5.c for more information.
 */

#ifndef LLVM_SYSTEM_MD5_H
#define LLVM_SYSTEM_MD5_H

#include "llvm/Support/DataTypes.h"

namespace llvm {

class MD5 {
  // Any 32-bit or wider unsigned integer data type will do.
  typedef uint32_t MD5_u32plus;

  MD5_u32plus a, b, c, d;
  MD5_u32plus hi, lo;
  unsigned char buffer[64];
  MD5_u32plus block[16];

 public:
  MD5();

  /// \brief Updates the hash for arguments provided.
  void Update(void *data, unsigned long size);

  /// \brief Finishes off the hash and puts the result in result.
  void Final(unsigned char *result);

private:
  void *body(void *data, unsigned long size);
};

}

#endif
