/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __TZSERIAL_SSL_SETUP_H__
#define __TZSERIAL_SSL_SETUP_H__

#include <openssl/ssl.h>

namespace tzrpc {

bool Ssl_thread_setup();
void Ssl_thread_clean();


extern SSL_CTX* global_ssl_ctx;


} // end namespace tzrpc

#endif //__TZSERIAL_SSL_SETUP_H__
