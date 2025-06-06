/*****************************************************************************\
 *  tls.h - tls API definitions
 *****************************************************************************
 *  Copyright (C) SchedMD LLC.
 *
 *  This file is part of Slurm, a resource management program.
 *  For details, see <https://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  Slurm is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  Slurm is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Slurm; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#ifndef _INTERFACES_TLS_H
#define _INTERFACES_TLS_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "src/common/slurm_time.h"

typedef enum {
	TLS_CONN_NULL = 0,
	TLS_CONN_SERVER,
	TLS_CONN_CLIENT,
} tls_conn_mode_t;

typedef struct {
	/* Function pointer type is the same as s2n_recv_fn */
	int (*recv)(void *io_context, uint8_t *buf, uint32_t len);
	/* Function pointer type is the same as s2n_send_fn */
	int (*send)(void *io_context, const uint8_t *buf, uint32_t len);

	/* Pointer to hand to recv() and send() callbacks */
	void *io_context;
} tls_conn_callbacks_t;

typedef struct {
	/* file descriptor for incoming data */
	int input_fd;
	/* file descriptor for outgoing data */
	int output_fd;
	/* TLS connection mode (@see tls_conn_mode_t) */
	tls_conn_mode_t mode;
	/*
	 * False: Enable any library based blinding delays
	 * True: Disable any library based blinding delays which caller will
	 *	need to be honored via call to tls_g_get_delay() after any
	 *	tls_g_*() failure
	 */
	bool defer_blinding;
	tls_conn_callbacks_t callbacks;
	/*
         * False: Attempt TLS negotiation in tls_g_create_conn()
         * True: Defer TLS negotiation in tls_g_create_conn() to explicit call
         *      to tls_g_nego_conn()
         */
        bool defer_negotiation;
	/*
	 * server certificate used by TLS_CONN_CLIENT connections when server
	 * certificate is not signed by a CA in our trust store
	 */
	char *cert;
} tls_conn_args_t;

extern char *tls_conn_mode_to_str(tls_conn_mode_t mode);

/*
 * Returns true if the default plugin is not tls/none.
 * The default plugin is used for RPC traffic.
 */
extern bool tls_enabled(void);

/*
 * Returns true if a plugin besides tls/none is supported.
 */
extern bool tls_supported(void);

extern int tls_g_init(void);
extern int tls_g_fini(void);

/*
 * Create new TLS connection
 * IN tls_conn_args - ptr to tls_conn_args_t
 * RET ptr to TLS state
 */
extern void *tls_g_create_conn(const tls_conn_args_t *tls_conn_args);
extern void tls_g_destroy_conn(void *conn);

/*
 * Attempt TLS connection negotiation
 * NOTE: Only to be called at start of connection and if defer_negotiation=true
 * RET SLURM_SUCCESS or EWOULDBLOCK or error
 */
extern int tls_g_negotiate_conn(void *conn);

/*
 * Set read/write fd's on TLS connection
 * NOTE: This resets send/recv callbacks/contexts in TLS connection
 * IN conn - TLS connection to reconfigure
 * IN input_fd - new read fd
 * IN output_fd - new write fd
 * RET SLURM_SUCCESS or error
 */
extern int tls_g_set_conn_fds(void *conn, int input_fd, int output_fd);

/*
 * Set read/write fd's on TLS connection
 * NOTE: This resets read/write fd's in TLS connection
 * IN conn - TLS connection to reconfigure
 * IN input_fd - new read fd
 * IN output_fd - new write fd
 * RET SLURM_SUCCESS or error
 */
extern int tls_g_set_conn_callbacks(void *conn,
				    tls_conn_callbacks_t *callbacks);

/*
 * Get absolute time that next tls_g_*() should be delayed until after any
 * failure
 * NOTE: returned timespec may be {0,0} indicating no delay required
 */
extern timespec_t tls_g_get_delay(void *conn);

extern ssize_t tls_g_send(void *conn, const void *buf, size_t n);
extern ssize_t tls_g_recv(void *conn, void *buf, size_t n);

/*
 * Check if buffer contains a TLS (or SSLv3) handshake
 * IN buf - pointer to buffer
 * IN n - number of bytes in buffer
 * IN name - connection name (for logging)
 * RET
 *	SLURM_SUCCESS: buffer contains TLS handshake
 *	ENOENT: buffer does not contain TLS handshake
 *	EWOULDBLOCK: buffer needs more bytes to determine match
 */
extern int tls_is_handshake(const void *buf, const size_t n, const char *name);

#endif
