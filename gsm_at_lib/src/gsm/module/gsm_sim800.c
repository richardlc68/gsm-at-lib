/**
 * \file            simcom_sim800.c
 * \brief           SIMcom sim800 device driver
 */

/*
 * Copyright (c) 2020
 *
 * Author:          Richard Liu (richardl@paradox.com)
 * Version:         $_version_$
 */
#include "gsm/module/simcom_sim800.h"
#include "gsm/gsm_private.h"
#include "gsm/gsm.h"
#include "gsm/gsm_int.h"
#include "gsm/gsm_mem.h"
#include "gsm/gsm_parser.h"
#include "gsm/gsm_unicode.h"
#include "system/gsm_ll.h"

/**
 * \brief           power on
 * \param[in]       pm: me_data
 */
static void
me_power_on(struct me_data* pm) {
    GSM_UNUSED(pm);

}

/**
 * \brief           power off
 * \param[in]       pm: me_data
 */
static void
me_power_off(struct me_data* pm) {
    GSM_UNUSED(pm);

}

/**
 * \brief           reset
 * \param[in]       pm: me_data
 */
static void
me_reset(struct me_data* pm) {
    GSM_UNUSED(pm);

}

static gsmr_t
gsmi_initiate_cmd(gsm_msg_t* msg) {
    switch (CMD_GET_CUR()) {                    /* Check current message we want to send over AT */
    case GSM_CMD_CIPSHUT: {                 /* Shut down network connection and put to reset state */
        AT_PORT_SEND_CONST_STR("AT+CIPSHUT" CRLF);
        break;
    }
    case GSM_CMD_CIPMUX: {                  /* Enable multiple connections */
        AT_PORT_SEND_CONST_STR("AT+CIPMUX=1" CRLF);
        break;
    }
    case GSM_CMD_CIPHEAD: {                 /* Enable information on receive data about connection and length */
        AT_PORT_SEND_CONST_STR("AT+CIPHEAD=1" CRLF);
        break;
    }
    case GSM_CMD_CIPSRIP: {
        AT_PORT_SEND_CONST_STR("AT+CIPSRIP=1" CRLF);
        break;
    }
    case GSM_CMD_CIPSSL: {                  /* Set SSL configuration */
        AT_PORT_SEND_CONST_STR("AT+CIPSSL=");
        gsmi_send_number((msg->msg.conn_start.type == GSM_CONN_TYPE_SSL) ? 1 : 0, 0, 0);
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_CIPSTART: {                /* Start a new connection */
        gsm_conn_t* c = NULL;

        /* Do we have network connection? */
        /* Check if we are connected to network */

        msg->msg.conn_start.num = 0;        /* Start with max value = invalidated */
        for (int16_t i = GSM_CFG_MAX_CONNS - 1; i >= 0; --i) {  /* Find available connection */
            if (!gsm.m.conns[i].status.f.active) {
                c = &gsm.m.conns[i];
                c->num = GSM_U8(i);
                msg->msg.conn_start.num = GSM_U8(i);    /* Set connection number for message structure */
                break;
            }
        }
        if (c == NULL) {
            gsmi_send_conn_error_cb(msg, gsmERRNOFREECONN);
            return gsmERRNOFREECONN;        /* We don't have available connection */
        }

        if (msg->msg.conn_start.conn != NULL) { /* Is user interested about connection info? */
            *msg->msg.conn_start.conn = c;  /* Save connection for user */
        }

        AT_PORT_SEND_CONST_STR("AT+CIPSTART=");
        gsmi_send_number(GSM_U32(c->num), 0, 0);
        if (msg->msg.conn_start.type == GSM_CONN_TYPE_TCP) {
            gsmi_send_string("TCP", 0, 1, 1);
        }
        else if (msg->msg.conn_start.type == GSM_CONN_TYPE_UDP) {
            gsmi_send_string("UDP", 0, 1, 1);
        }
        gsmi_send_string(msg->msg.conn_start.host, 0, 1, 1);
        gsmi_send_port(msg->msg.conn_start.port, 0, 1);
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_CIPCLOSE: {          /* Close the connection */
        gsm_conn_p c = msg->msg.conn_close.conn;
        if (c != NULL &&
            /* Is connection already closed or command for this connection is not valid anymore? */
            (!gsm_conn_is_active(c) || c->val_id != msg->msg.conn_close.val_id)) {
            return gsmERR;
        }
        AT_PORT_SEND_CONST_STR("AT+CIPCLOSE=");
        gsmi_send_number(GSM_U32(msg->msg.conn_close.conn ? msg->msg.conn_close.conn->num : GSM_CFG_MAX_CONNS), 0, 0);
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_CIPSEND: {                 /* Send data to connection */
        return gsmi_tcpip_process_send_data();  /* Process send data */
    }
    case GSM_CMD_CIPSTATUS: {               /* Get status of device and all connections */
        AT_PORT_SEND_CONST_STR("AT+CIPSTATUS" CRLF);
        break;
    }
#if GSM_CFG_SMS
    case GSM_CMD_CMGDA: {                   /* Mass delete SMS messages */
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("+CMGDA=");
        switch (msg->msg.sms_delete_all.status) {
        case GSM_SMS_STATUS_READ:   gsmi_send_string("DEL READ", 0, 1, 0); break;
        case GSM_SMS_STATUS_UNREAD: gsmi_send_string("DEL UNREAD", 0, 1, 0); break;
        case GSM_SMS_STATUS_SENT:   gsmi_send_string("DEL SENT", 0, 1, 0); break;
        case GSM_SMS_STATUS_UNSENT: gsmi_send_string("DEL UNSENT", 0, 1, 0); break;
        case GSM_SMS_STATUS_INBOX:  gsmi_send_string("DEL INBOX", 0, 1, 0); break;
        case GSM_SMS_STATUS_ALL:    gsmi_send_string("DEL ALL", 0, 1, 0); break;
        default: break;
        }
        AT_PORT_SEND_END_AT();
        break;
    }

#endif /* GSM_CFG_SMS */
#if GSM_CFG_CALL
    case GSM_CMD_ATD: {                     /* Start new call */
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("D");
        gsmi_send_string(msg->msg.call_start.number, 0, 0, 0);
        AT_PORT_SEND_CONST_STR(";");
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_ATA: {                     /* Answer phone call */
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("A");
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_ATH: {                     /* Disconnect existing connection (hang-up phone call) */
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("H");
        AT_PORT_SEND_END_AT();
        break;
    }
#endif /* GSM_CFG_CALL */
    case GSM_CMD_NETWORK_DETACH:
    case GSM_CMD_CGATT_SET_0: {
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("+CGATT=0");
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_CGATT_SET_1: {
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("+CGATT=1");
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_CIPMUX_SET: {
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("+CIPMUX=1");
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_CIPRXGET_SET: {
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("+CIPRXGET=0");
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_CSTT_SET: {
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("+CSTT=");
        gsmi_send_string(msg->msg.network_attach.apn, 1, 1, 0);
        gsmi_send_string(msg->msg.network_attach.user, 1, 1, 1);
        gsmi_send_string(msg->msg.network_attach.pass, 1, 1, 1);
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_CIICR: {
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("+CIICR");
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_CIFSR: {
        AT_PORT_SEND_BEGIN_AT();
        AT_PORT_SEND_CONST_STR("+CIFSR");
        AT_PORT_SEND_END_AT();
        break;
    }
    default:
        return gsmERR;                      /* Invalid command */
    }
    return gsmOK;                               /* Valid command */
}

/**
 * \brief           Process current command with known execution status and start another if necessary
 * \param[in]       msg: Pointer to current message
 * \param[in]       is_ok: pointer to status whether last command result was OK
 * \param[in]       is_error: Pointer to status whether last command result was ERROR
 * \return          \ref gsmCONT if you sent more data and we need to process more data, \ref gsmOK on success, or \ref gsmERR on error
 */
static gsmr_t
gsmi_process_sub_cmd(gsm_msg_t* msg, uint8_t* is_ok, uint16_t* is_error) {
    gsm_cmd_t n_cmd = GSM_CMD_IDLE;

    if(gsm.msg == NULL) return gsmERR;

    switch (gsm.msg->cmd_def) {
        /* reset me */
    case GSM_CMD_RESET:
        switch (CMD_GET_CUR()) {                /* Check current command */
            case GSM_CMD_CPIN_GET:
#if defined(RELEASE)
                SET_NEW_CMD(GSM_CMD_IPR); break;
            case GSM_CMD_IPR: 
                gsm_delay(500);
#endif
                SET_NEW_CMD(GSM_CMD_CLCC_SET); /* Set call state */
                break;
            default: break;
        }
        if (n_cmd == GSM_CMD_IDLE) {
            RESET_SEND_EVT(msg, gsmOK);
        }
        break;

        /* attach network */
    case GSM_CMD_NETWORK_ATTACH:
        switch (msg->i) {
            case 0: SET_NEW_CMD_CHECK_ERROR(GSM_CMD_CGACT_SET_0); break;
            case 1: SET_NEW_CMD(GSM_CMD_CGACT_SET_1); break;
#if GSM_CFG_NETWORK_IGNORE_CGACT_RESULT
            case 2: SET_NEW_CMD(GSM_CMD_CGATT_SET_0); break;
#else /* GSM_CFG_NETWORK_IGNORE_CGACT_RESULT */
            case 2: SET_NEW_CMD_CHECK_ERROR(GSM_CMD_CGATT_SET_0); break;
#endif /* !GSM_CFG_NETWORK_IGNORE_CGACT_RESULT */
            case 3: SET_NEW_CMD(GSM_CMD_CGATT_SET_1); break;
            case 4: SET_NEW_CMD_CHECK_ERROR(GSM_CMD_CIPSHUT); break;
            case 5: SET_NEW_CMD_CHECK_ERROR(GSM_CMD_CIPMUX_SET); break;
            case 6: SET_NEW_CMD_CHECK_ERROR(GSM_CMD_CIPRXGET_SET); break;
            case 7: SET_NEW_CMD_CHECK_ERROR(GSM_CMD_CSTT_SET); break;
            case 8: SET_NEW_CMD_CHECK_ERROR(GSM_CMD_CIICR); break;
            case 9: SET_NEW_CMD_CHECK_ERROR(GSM_CMD_CIFSR); break;
            case 10: SET_NEW_CMD(GSM_CMD_CIPSTATUS); break;
            default: break;
        }
        break;

        /* detach network */
    case GSM_CMD_NETWORK_DETACH:
        switch (msg->i) {
            case 0: SET_NEW_CMD(GSM_CMD_CGATT_SET_0); break;
            case 1: SET_NEW_CMD(GSM_CMD_CGACT_SET_0); break;
            case 2: SET_NEW_CMD(GSM_CMD_CIPSTATUS); break;
            default: break;
        }
        if (!n_cmd) {
            *is_ok = 1;
        }
        break;

    case GSM_CMD_SOCKET_OPEN:
        if (!msg->i && CMD_IS_CUR(GSM_CMD_CIPSTATUS)) { /* Was the current command status info? */
            if (*is_ok) {
                SET_NEW_CMD(GSM_CMD_CIPSSL);    /* Set SSL */
            }
        } else if (msg->i == 1 && CMD_IS_CUR(GSM_CMD_CIPSSL)) {
            SET_NEW_CMD(GSM_CMD_CIPSTART);      /* Now actually start connection */
        } else if (msg->i == 2 && CMD_IS_CUR(GSM_CMD_CIPSTART)) {
            SET_NEW_CMD(GSM_CMD_CIPSTATUS);     /* Go to status mode */
            if (*is_error) {
                msg->msg.conn_start.conn_res = GSM_CONN_CONNECT_ERROR;
            }
        } else if (msg->i == 3 && CMD_IS_CUR(GSM_CMD_CIPSTATUS)) {
            /* After second CIP status, define what to do next */
            switch (msg->msg.conn_start.conn_res) {
                case GSM_CONN_CONNECT_OK: {     /* Successfully connected */
                    gsm_conn_t* conn = &gsm.m.conns[msg->msg.conn_start.num];   /* Get connection number */

                    gsm.evt.type = GSM_EVT_CONN_ACTIVE; /* Connection just active */
                    gsm.evt.evt.conn_active_close.client = 1;
                    gsm.evt.evt.conn_active_close.conn = conn;
                    gsm.evt.evt.conn_active_close.forced = 1;
                    gsmi_send_conn_cb(conn, NULL);
                    gsmi_conn_start_timeout(conn);  /* Start connection timeout timer */
                    break;
                }
                case GSM_CONN_CONNECT_ERROR: {  /* Connection error */
                    gsmi_send_conn_error_cb(msg, gsmERRCONNFAIL);
                    *is_error = 1;              /* Manually set error */
                    *is_ok = 0;                 /* Reset success */
                    break;
                }
                default: {
                    /* Do nothing as of now */
                    break;
                }
            }
        }
        break;

    case GSM_CMD_SOCKET_CLOSE:
        /*
         * It is unclear in which state connection is when ERROR is received on close command.
         * Stack checks if connection is closed before it allows and sends close command,
         * however it was detected that no automatic close event has been received from device
         * and AT+CIPCLOSE returned ERROR.
         *
         * Is it device firmware bug?
         */
        if (CMD_IS_CUR(GSM_CMD_CIPCLOSE) && *is_error) {
            /* Notify upper layer about failed close event */
            gsm.evt.type = GSM_EVT_CONN_CLOSE;
            gsm.evt.evt.conn_active_close.conn = msg->msg.conn_close.conn;
            gsm.evt.evt.conn_active_close.forced = 1;
            gsm.evt.evt.conn_active_close.res = gsmERR;
            gsm.evt.evt.conn_active_close.client = msg->msg.conn_close.conn->status.f.active && msg->msg.conn_close.conn->status.f.client;
            gsmi_send_conn_cb(msg->msg.conn_close.conn, NULL);
        }
        break;
        
    }

    /* Check if new command was set for execution */
    if (n_cmd != GSM_CMD_IDLE) {
        gsmr_t res;
        msg->cmd = n_cmd;
        if ((res = msg->fn(msg)) == gsmOK) {
            return gsmCONT;
        } else {
            *is_ok = 0;
            *is_error = 1;
            return res;
        }
    } else {
        msg->cmd = GSM_CMD_IDLE;
    }
    return *is_ok ? gsmOK : gsmERR;
}

/**
 * \brief           Parse connection info line from CIPSTATUS command
 * \param[in]       str: Input string
 * \param[in]       is_conn_line: Set to `1` for connection, `0` for general status
 * \param[out]      continueScan: Pointer to output variable holding continue processing state
 * \return          `1` on success, `0` otherwise
 */
static uint8_t
gsmi_parse_socket_conn(const char* str, uint8_t is_conn_line, uint8_t* continueScan) {
    uint8_t num;
    gsm_conn_t* conn;
    char s_tmp[16];
    uint8_t tmp_pdp_state;

    *continueScan = 1;
    if (is_conn_line && (*str == 'C' || *str == 'S')) {
        str += 3;
    } else {
        /* Check if PDP context is deactivated or not */
        tmp_pdp_state = 1;
        if (!strncmp(&str[7], "IP INITIAL", 10)) {
            *continueScan = 0;                  /* Stop command execution at this point (no OK,ERROR received after this line) */
            tmp_pdp_state = 0;
        } else if (!strncmp(&str[7], "PDP DEACT", 9)) {
            /* Deactivated */
            tmp_pdp_state = 0;
        }

        /* Check if we have to update status for application */
        if (gsm.m.network.is_attached != tmp_pdp_state) {
            gsm.m.network.is_attached = tmp_pdp_state;

            /* Notify upper layer */
            gsmi_send_cb(gsm.m.network.is_attached ? GSM_EVT_NETWORK_ATTACHED : GSM_EVT_NETWORK_DETACHED);
        }

        return 1;
    }

    /* Parse connection line */
    num = GSM_U8(gsmi_parse_number(&str));
    conn = &gsm.m.conns[num];

    conn->status.f.bearer = GSM_U8(gsmi_parse_number(&str));
    gsmi_parse_string(&str, s_tmp, sizeof(s_tmp), 1);   /* Parse TCP/UPD */
    if (strlen(s_tmp)) {
        if (!strcmp(s_tmp, "TCP")) {
            conn->type = GSM_CONN_TYPE_TCP;
        } else if (!strcmp(s_tmp, "UDP")) {
            conn->type = GSM_CONN_TYPE_UDP;
        }
    }
    gsmi_parse_ip(&str, &conn->remote_ip);
    conn->remote_port = gsmi_parse_number(&str);

    /* Get connection status */
    gsmi_parse_string(&str, s_tmp, sizeof(s_tmp), 1);

    /* TODO: Implement all connection states */
    if (!strcmp(s_tmp, "INITIAL")) {

    } else if (!strcmp(s_tmp, "CONNECTING")) {

    } else if (!strcmp(s_tmp, "CONNECTED")) {

    } else if (!strcmp(s_tmp, "REMOTE CLOSING")) {

    } else if (!strcmp(s_tmp, "CLOSING")) {

    } else if (!strcmp(s_tmp, "CLOSED")) {      /* Connection closed */
        if (conn->status.f.active) {            /* Check if connection is not */
            gsmi_conn_closed_process(conn->num, 0); /* Process closed event */
        }
    }

    /* Save last parsed connection */
    gsm.m.active_conns_cur_parse_num = num;

    return 1;
}

static void gsmi_parse_socket_sta(uint8_t* is_ok, gsm_recv_t* rcv)
{
    /* OK is returned before important data */
    *is_ok = 0;
    /* Check if connection data received */
    if (rcv->len > 3) {
        uint8_t continueScan, processed = 0;
        if (rcv->data[0] == 'C' && rcv->data[1] == ':' && rcv->data[2] == ' ') {
            processed = 1;
            gsmi_parse_socket_conn(rcv->data, 1, &continueScan);

            if (gsm.m.active_conns_cur_parse_num == (GSM_CFG_MAX_CONNS - 1)) {
                *is_ok = 1;
            }
        } else if (!strncmp(rcv->data, "STATE:", 6)) {
            processed = 1;
            gsmi_parse_socket_conn(rcv->data, 0, &continueScan);
        }

        /* Check if we shall stop processing at this stage */
        if (processed && !continueScan) {
            *is_ok = 1;
        }
    }

}

/**
 * \brief           Process received string from GSM
 * \param[in]       rcv: Pointer to \ref gsm_recv_t structure with input string
 */
static void
gsmi_parse_received_plus(uint8_t *is_ok, uint16_t *is_error, gsm_recv_t* rcv) {
    #if 0
    uint8_t is_ok = 0;
    uint16_t is_error = 0;

    /* Scan received strings which start with '+' */
    if (!strncmp(rcv->data, "+QNWINFO", 8)) {
        gsmi_parse_nwinfo(rcv->data);          /* Parse +QNWINFO response */
    } else if (!strncmp(rcv->data, "+PDP: DEACT", 11)) {
        /* PDP has been deactivated */
        gsm_network_check_status(NULL, NULL, 0);/* Update status */
    } else if (!strncmp(rcv->data, "+RECEIVE", 8)) {
        gsmi_parse_ipd(rcv->data);          /* Parse IPD */
    } else if (!strncmp(rcv->data, "+CPIN", 5)) {   /* Check for +CPIN indication for SIM */
        gsmi_parse_cpin(rcv->data, 1 /* !CMD_IS_DEF(GSM_CMD_CPIN_SET) */);  /* Parse +CPIN response */
    } else if (CMD_IS_CUR(GSM_CMD_COPS_GET) && !strncmp(rcv->data, "+COPS", 5)) {
        gsmi_parse_cops(rcv->data);         /* Parse current +COPS */
    } 
    /* Check general responses for active commands */
    if (gsm.msg != NULL) {
        if (0) {
#if GSM_CFG_SMS
        } else if (CMD_IS_CUR(GSM_CMD_CMGS) && is_ok) {
            /* At this point we have to wait for "> " to send data */
#endif /* GSM_CFG_SMS */
        } else if (CMD_IS_CUR(GSM_CMD_CIPSTATUS)) {
            /* For CIPSTATUS, OK is returned before important data */
            if (is_ok) {
                is_ok = 0;
            }
            /* Check if connection data received */
            if (rcv->len > 3) {
                uint8_t continueScan, processed = 0;
                if (rcv->data[0] == 'C' && rcv->data[1] == ':' && rcv->data[2] == ' ') {
                    processed = 1;
                    gsmi_parse_socket_conn(rcv->data, 1, &continueScan);

                    if (gsm.m.active_conns_cur_parse_num == (GSM_CFG_MAX_CONNS - 1)) {
                        is_ok = 1;
                    }
                } else if (!strncmp(rcv->data, "STATE:", 6)) {
                    processed = 1;
                    gsmi_parse_socket_conn(rcv->data, 0, &continueScan);
                }

                /* Check if we shall stop processing at this stage */
                if (processed && !continueScan) {
                    is_ok = 1;
                }
            }
        } else if (CMD_IS_CUR(GSM_CMD_CIPSTART)) {
            /* For CIPSTART, OK is returned before important data */
            if (is_ok) {
                is_ok = 0;
            }

            /* Wait here for CONNECT status before we cancel connection */
            if (GSM_CHARISNUM(rcv->data[0])
                && rcv->data[1] == ',' && rcv->data[2] == ' ') {
                uint8_t num = GSM_CHARTONUM(rcv->data[0]);
                if (num < GSM_CFG_MAX_CONNS) {
                    uint8_t id;
                    gsm_conn_t* conn = &gsm.m.conns[num];   /* Get connection handle */

                    if (!strncmp(&rcv->data[3], "CONNECT OK" CRLF, 10 + CRLF_LEN)) {
                        id = conn->val_id;
                        GSM_MEMSET(conn, 0x00, sizeof(*conn));  /* Reset connection parameters */
                        conn->num = num;
                        conn->status.f.active = 1;
                        conn->val_id = ++id;            /* Set new validation ID */

                        /* Set connection parameters */
                        conn->status.f.client = 1;
                        conn->evt_func = gsm.msg->msg.conn_start.evt_func;
                        conn->arg = gsm.msg->msg.conn_start.arg;

                        /* Set status */
                        gsm.msg->msg.conn_start.conn_res = GSM_CONN_CONNECT_OK;
                        is_ok = 1;
                    } else if (!strncmp(&rcv->data[3], "CONNECT FAIL" CRLF, 12 + CRLF_LEN)) {
                        gsm.msg->msg.conn_start.conn_res = GSM_CONN_CONNECT_ERROR;
                        is_error = 1;
                    } else if (!strncmp(&rcv->data[3], "ALREADY CONNECT" CRLF, 15 + CRLF_LEN)) {
                        gsm.msg->msg.conn_start.conn_res = GSM_CONN_CONNECT_ALREADY;
                        is_error = 1;
                    }
                }
            }
        } else if (CMD_IS_CUR(GSM_CMD_CIPSEND)) {
            if (is_ok) {
                is_ok = 0;
            }
            gsmi_process_cipsend_response(rcv, &is_ok, &is_error);
#if GSM_CFG_USSD
        } else if (CMD_IS_CUR(GSM_CMD_CUSD)) {
            /* OK is returned before +CUSD */
            /* Command is not finished yet, unless it was an ERROR */
            if (is_ok) {
                is_ok = 0;
            }

            /* Check for manual CUSTOM OK message */
            if (!strcmp(rcv->data, "CUSTOM_OK\r\n")) {
                is_ok = 1;
            }
#endif /* GSM_CFG_USSD */
        }
    }

    /*
     * In case of any of these events, simply release semaphore
     * and proceed with next command
     */
    if (is_ok || is_error) {
        gsmr_t res = gsmOK;
        if (gsm.msg != NULL) {                  /* Do we have active message? */
            res = gsmi_process_sub_cmd(gsm.msg, &is_ok, &is_error);
            if (res != gsmCONT) {               /* Shall we continue with next subcommand under this one? */
                if (is_ok) {                    /* Check OK status */
                    res = gsm.msg->res = gsmOK;
                } else {                        /* Or error status */
                    res = gsm.msg->res = res;   /* Set the error status */
                }
            } else {
                ++gsm.msg->i;                   /* Number of continue calls */
            }

            /*
             * When the command is finished,
             * release synchronization semaphore
             * from user thread and start with next command
             */
            if (res != gsmCONT) {                   /* Do we have to continue to wait for command? */
                gsm_sys_sem_release(&gsm.sem_sync); /* Release semaphore */
            }
        }
    }
#endif
}

/* instance platform */
const struct me_data sim800 = {
    .GSM_CMD_CGACT_SET_0      = "AT+CGACT=0" CRLF ,
    .GSM_CMD_CGACT_SET_1      = "AT+CGACT=1" CRLF ,
    .GSM_CMD_SOCKET_OPEN      = GSM_CMD_CIPSTART,
    .GSM_CMD_SOCKET_SEND      = GSM_CMD_CIPSEND,
    .GSM_CMD_SOCKET_CLOSE     = GSM_CMD_CIPCLOSE,
    .GSM_CMD_SOCKET_STA       = GSM_CMD_CIPSTATUS,
    .creg_cgreg_skip_first    = 0,
    .high_baudrate            = 460800,
    .reset_gpio               = 100,
    .power_on_gpio            = 101,
    .power_on                 = me_power_on,
    .power_off                = me_power_off,
    .reset                    = me_reset,
    .gsmi_initiate_cmd        = gsmi_initiate_cmd,
    .gsmi_process_sub_cmd     = gsmi_process_sub_cmd,
    .gsmi_parse_received_plus = gsmi_parse_received_plus,
    .gsmi_parse_socket_sta    = gsmi_parse_socket_sta,

};
