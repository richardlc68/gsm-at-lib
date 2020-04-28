/**
 * \file            gsm_int.h
 * \brief           Internal functions
 */

/*
 * Copyright (c) 2020 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of GSM-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         $_version_$
 */
#ifndef GSM_HDR_INT_H
#define GSM_HDR_INT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gsm/gsm_private.h"

#if !__DOXYGEN__
/* Receive character macros */
#define RECV_ADD(ch)                        do { if (recv_buff.len < (sizeof(recv_buff.data)) - 1) { recv_buff.data[recv_buff.len++] = ch; recv_buff.data[recv_buff.len] = 0; } } while (0)
#define RECV_RESET()                        do { recv_buff.len = 0; recv_buff.data[0] = 0; } while (0)
#define RECV_LEN()                          ((size_t)recv_buff.len)
#define RECV_IDX(index)                     recv_buff.data[index]

/* Send data over AT port */
#define AT_PORT_SEND_STR(str)               gsm.ll.send_fn((const void *)(str), (size_t)strlen(str))
#define AT_PORT_SEND_CONST_STR(str)         gsm.ll.send_fn((const void *)(str), (size_t)(sizeof(str) - 1))
#define AT_PORT_SEND_CHR(ch)                gsm.ll.send_fn((const void *)(ch), (size_t)1)
#define AT_PORT_SEND_FLUSH()                gsm.ll.send_fn(NULL, 0)
#define AT_PORT_SEND(d, l)                  gsm.ll.send_fn((const void *)(d), (size_t)(l))
#define AT_PORT_SEND_WITH_FLUSH(d, l)       do { AT_PORT_SEND((d), (l)); AT_PORT_SEND_FLUSH(); } while (0)

/* Beginning and end of every AT command */
#define AT_PORT_SEND_BEGIN_AT()             do { AT_PORT_SEND_CONST_STR("AT"); } while (0)
#define AT_PORT_SEND_END_AT()               do { AT_PORT_SEND(CRLF, CRLF_LEN); AT_PORT_SEND(NULL, 0); } while (0)

/* Send special characters over AT port with condition */
#define AT_PORT_SEND_QUOTE_COND(q)          do { if ((q)) { AT_PORT_SEND_CONST_STR("\""); } } while (0)
#define AT_PORT_SEND_COMMA_COND(c)          do { if ((c)) { AT_PORT_SEND_CONST_STR(","); } } while (0)
#define AT_PORT_SEND_EQUAL_COND(e)          do { if ((e)) { AT_PORT_SEND_CONST_STR("="); } } while (0)

/* Send special characters */
#define AT_PORT_SEND_CTRL_Z()               AT_PORT_SEND_STR("\x1A")
#define AT_PORT_SEND_ESC()                  AT_PORT_SEND_STR("\x1B")
#endif /* !__DOXYGEN__ */

/**
 * \brief           Send string to AT port, either plain or escaped
 * \param[in]       str: Pointer to input string to string
 * \param[in]       e: Value to indicate string send format, escaped (`1`) or plain (`0`)
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (`1`) or disabled (`0`)
 * \param[in]       c: Set to `1` to include comma before string
 */
void
gsmi_send_string(const char* str, uint8_t e, uint8_t q, uint8_t c);

/**
 * \brief           Send number (decimal) to AT port
 * \param[in]       num: Number to send to AT port
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (`1`) or disabled (`0`)
 * \param[in]       c: Set to `1` to include comma before string
 */
void gsmi_send_number(uint32_t num, uint8_t q, uint8_t c);

/**
 * \brief           Send port number to AT port
 * \param[in]       port: Port number to send
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (`1`) or disabled (`0`)
 * \param[in]       c: Set to `1` to include comma before string
 */
void gsmi_send_port(gsm_port_t port, uint8_t q, uint8_t c);

/**
 * \brief           Send signed number to AT port
 * \param[in]       num: Number to send to AT port
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (`1`) or disabled (`0`)
 * \param[in]       c: Set to `1` to include comma before string
 */
void gsmi_send_signed_number(int32_t num, uint8_t q, uint8_t c);

/**
 * \brief           Send memory string to device
 * \param[in]       mem: Memory index to send
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (`1`) or disabled (`0`)
 * \param[in]       c: Set to `1` to include comma before string
 */
void gsmi_send_dev_memory(gsm_mem_t mem, uint8_t q, uint8_t c);

/**
 * \brief           Send error event to application layer
 * \param[in]       msg: Message from user with connection start
 * \param[in]       error: Error type
 */
void gsmi_send_conn_error_cb(gsm_msg_t *msg, gsmr_t error);

/**
 * \brief           Process and send data from device buffer
 * \return          Member of \ref gsmr_t enumeration
 */
gsmr_t gsmi_tcpip_process_send_data(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GSM_HDR_INT_H */
