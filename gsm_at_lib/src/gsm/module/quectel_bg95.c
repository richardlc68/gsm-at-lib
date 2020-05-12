/**
 * \file            gsm_bg95.c
 * \brief           Quectel BG95 device driver
 */

/*
 * Copyright (c) 2020
 *
 * Author:          Richard Liu (richardl@paradox.com)
 * Version:         $_version_$
 */
#include "gsm/module/quectel_bg95.h"
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
    char bad_idea[256];
    switch (CMD_GET_CUR()) {                    /* Check current message we want to send over AT */
    case GSM_CMD_QCFG_BAND:
        AT_PORT_SEND_CONST_STR("AT+QCFG=\"band\",F,100002000000000F0E389F,100042000000000B0E189F,1" CRLF);
        break;
    case GSM_CMD_QCFG_NWSCANMODE:
        AT_PORT_SEND_CONST_STR("AT+QCFG=\"nwscanmode\",0,1" CRLF);
        break;
    case GSM_CMD_QCFG_NWSCANSEQ:
        AT_PORT_SEND_CONST_STR("AT+QCFG=\"nwscanseq\",00" CRLF);
        break;
    case GSM_CMD_QCFG_TCP_RETRANSCFG:
        AT_PORT_SEND_CONST_STR("AT+QICFG=\"tcp/retranscfg\",20,200" CRLF);
        break;
    case GSM_CMD_QNWINFO:
        AT_PORT_SEND_CONST_STR("AT+QNWINFO" CRLF);
        break;    
    case GSM_CMD_QIACT_SET:
        AT_PORT_SEND_CONST_STR("AT+QIACT=1" CRLF);
        break;   
    case GSM_CMD_QIACT_GET:
        AT_PORT_SEND_CONST_STR("AT+QIACT?" CRLF);
        break;   
    case GSM_CMD_QICSGP:
        sprintf(bad_idea, "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\"" CRLF,
            msg->msg.network_attach.apn,
            msg->msg.network_attach.user,
            msg->msg.network_attach.pass
        );
        AT_PORT_SEND_STR(bad_idea);
        break;

    case GSM_CMD_CIPSSL: {                  /* Set SSL configuration */
        AT_PORT_SEND_CONST_STR("AT+CIPSSL=");
        gsmi_send_number((msg->msg.conn_start.type == GSM_CONN_TYPE_SSL) ? 1 : 0, 0, 0);
        AT_PORT_SEND_END_AT();
        break;
    }
    case GSM_CMD_QIOPEN: {                /* Start a new connection */
        gsm_conn_t* c = NULL;

        /* Do we have network connection? */
        /* Check if we are connected to network */

        msg->msg.conn_start.num = 0;        /* Start with max value = invalidated */
        for (int16_t i = 0; i<GSM_CFG_MAX_CONNS; i++) {  /* Find available connection */
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

        sprintf(bad_idea, "AT+QIOPEN=1,%d,\"%s\",\"%s\",%d,%d" CRLF,
                msg->msg.conn_start.num,
                (msg->msg.conn_start.type == GSM_CONN_TYPE_TCP) ? "TCP" : "UDP",
                msg->msg.conn_start.host,
                msg->msg.conn_start.port,
                0 //If <service_type> is "TCP LISTENER" or "UDP SERVICE", this parameter must be specified.
                  //If <service_type> is "TCP" or "UDP": the local port will be assigned automatically if <local_port> is 0
        );
        AT_PORT_SEND_STR(bad_idea);
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
    case GSM_CMD_QISTATE: {               /* Get status of device and all connections */
        AT_PORT_SEND_CONST_STR("AT+QISTATE" CRLF);
        break;
    }

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
                SET_NEW_CMD(GSM_CMD_IPR); 
                break;
            case GSM_CMD_IPR: 
                gsm_delay(500);
#endif
                SET_NEW_CMD(GSM_CMD_QCFG_NWSCANMODE); 
                break;
            case GSM_CMD_QCFG_NWSCANMODE: 
                SET_NEW_CMD(GSM_CMD_QCFG_NWSCANSEQ); 
                break;
            case GSM_CMD_QCFG_NWSCANSEQ: 
                SET_NEW_CMD(GSM_CMD_QCFG_BAND); 
                break;
            case GSM_CMD_QCFG_BAND: 
                SET_NEW_CMD(GSM_CMD_QCFG_TCP_RETRANSCFG); 
                break;
            case GSM_CMD_QCFG_TCP_RETRANSCFG: 
                SET_NEW_CMD(GSM_CMD_ATS10); 
                break;
            default: break;
        }
        if (n_cmd == GSM_CMD_IDLE) {
            RESET_SEND_EVT(msg, gsmOK);
        }
        break;

        /* attach network */
    case GSM_CMD_NETWORK_ATTACH:
        switch (CMD_GET_CUR()) {                /* Check current command */
            case GSM_CMD_NETWORK_ATTACH:
                SET_NEW_CMD_CHECK_ERROR(GSM_CMD_CGREG_GET); 
                break;
            case GSM_CMD_CGREG_GET:
                /* no need lock because this call is from gsmi_process() which already had */
                if ((gsm.m.network.egprs == GSM_NETWORK_REG_EGPRS_STATUS_CONNECTED) ||
                    (gsm.m.network.egprs == GSM_NETWORK_REG_EGPRS_STATUS_UNKNOWN) ||
                    (gsm.m.network.egprs == GSM_NETWORK_REG_EGPRS_STATUS_CONNECTED_ROAMING)
                    ) {
                    SET_NEW_CMD(GSM_CMD_QNWINFO);
                }
                else {
                    gsm_delay(3000);
                    SET_NEW_CMD(GSM_CMD_CGREG_GET);
                }
                break;
            case GSM_CMD_QNWINFO: SET_NEW_CMD(GSM_CMD_QICSGP);  break; /* set apn         */
            case GSM_CMD_QICSGP:  SET_NEW_CMD(GSM_CMD_QIACT_SET);   break; /* active PDP      */
            case GSM_CMD_QIACT_SET:   SET_NEW_CMD(GSM_CMD_QIACT_GET); break; /* Successfully activate PDP context? */
            case GSM_CMD_QIACT_GET:
                if (!gsm.m.network.is_attached) {
                    gsm_delay(100);
                    SET_NEW_CMD(GSM_CMD_QIACT_SET);
                }
                break;
        }
        break;

        /* detach network */
    case GSM_CMD_NETWORK_DETACH:
        switch (msg->i) {
            case 0: SET_NEW_CMD(GSM_CMD_CGATT_SET_0); break;
            case 1: SET_NEW_CMD(GSM_CMD_CGACT_SET_0); break;
            case 2: SET_NEW_CMD(GSM_CMD_QISTATE); break;
            default: break;
        }
        if (!n_cmd) {
            *is_ok = 1;
        }
        break;

    case GSM_CMD_QIOPEN:
        if (!msg->i && CMD_IS_CUR(GSM_CMD_QISTATE)) { /* Was the current command status info? */
            if (*is_ok) {
                SET_NEW_CMD(GSM_CMD_QIOPEN);      /* Now actually start connection */
            }
        } 
        else if (msg->i == 1 && CMD_IS_CUR(GSM_CMD_QIOPEN)) {
            gsm_delay(100);
            SET_NEW_CMD(GSM_CMD_QISTATE);     /* Go to status mode */
            if (*is_error) {
                msg->msg.conn_start.conn_res = GSM_CONN_CONNECT_ERROR;
            }
        } else if (msg->i < 3 && CMD_IS_CUR(GSM_CMD_QISTATE)) {
            /* After second QISTATE status, define what to do next */
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
                    gsm_delay(100);
                    SET_NEW_CMD(GSM_CMD_QISTATE);     /* Go to status mode */
                    break;
                }
            }
        }
        else {
            msg->i=100;
            //test
        }
        break;

    case GSM_CMD_QICLOSE:
        /*
         * It is unclear in which state connection is when ERROR is received on close command.
         * Stack checks if connection is closed before it allows and sends close command,
         * however it was detected that no automatic close event has been received from device
         * and AT+CIPCLOSE returned ERROR.
         *
         * Is it device firmware bug?
         */
        if (CMD_IS_CUR(GSM_CMD_QICLOSE) && *is_error) {
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
 * \brief           Parse received +CREG message
 * \param[in]       str: Input string to parse from
 * \return          1 on success, 0 otherwise
 */
static uint8_t
gsmi_parse_nwinfo(const char* str) {
    str += 10; /* +QNWINFO: "FDD LTE","302720","LTE BAND 12",5060*/
    gsmi_parse_string(&str, gsm.m.network.info, sizeof(gsm.m.network.info), 1);
    gsm.evt.evt.network.info = gsm.m.network.info;
    gsmi_send_cb(GSM_EVT_NETWORK_INFO);

    return 1;
}

/**
 * \brief           Parse connection info line from CIPSTATUS command
 * \param[in]       str: Input string
 * \param[out]      continueScan: Pointer to output variable holding continue processing state
 * \return          `1` on success, `0` otherwise
 */
static uint8_t
gsmi_parse_socket_conn(const char* str, uint8_t* continueScan) {
    uint8_t num;
    gsm_conn_t* conn;
    char s_tmp[16];
    uint8_t tmp_pdp_state;

    *continueScan = 1;
    str += 10;

    /* Parse connection line */
    num = GSM_U8(gsmi_parse_number(&str));
    conn = &gsm.m.conns[num];

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
    conn->local_port = gsmi_parse_number(&str);

    /* Get connection status */
    tmp_pdp_state=gsmi_parse_number(&str);
    switch(tmp_pdp_state) {
        case 0: //INITIAL
            break;
        case 1: //Connecting
            break;
        case 2: //Connected
            break;
        case 3: //Listening
            break;
        case 4: //Closing
            break;
    }
//    if (!strcmp(s_tmp, "CLOSED")) {      /* Connection closed */
//        if (conn->status.f.active) {            /* Check if connection is not */
//            gsmi_conn_closed_process(conn->num, 0); /* Process closed event */
//        }
//    }

    /* Save last parsed connection */
    gsm.m.active_conns_cur_parse_num = num;

    return 1;
}

static void gsmi_parse_qiact(uint8_t *is_ok, const char* str)
{
    str += 8; /* +QIACT: 1,1,1,"25.106.234.47" */
    gsmi_parse_number(&str); /* skip contextID*/
    uint8_t tmp_pdp_state = gsmi_parse_number(&str); /* context_state == 1 (actived) */
    gsmi_parse_number(&str); /* skip context_type (1=IPv4)*/
    gsmi_parse_ip(&str, &gsm.m.network.ip_addr);
    /* Check if we have to update status for application */
    if (gsm.m.network.is_attached != tmp_pdp_state) {
        gsm.m.network.is_attached = tmp_pdp_state;
        /* Notify upper layer */
        gsmi_send_cb(gsm.m.network.is_attached ? GSM_EVT_NETWORK_ATTACHED : GSM_EVT_NETWORK_DETACHED);
    }
    *is_ok = 1;
}

static void gsmi_parse_qiopen(uint16_t *is_error, const char* str)
{
    str += 9; /* +QIOPEN: 0,0 */
    gsmi_parse_number(&str); /* skip connectID */
    int32_t err = (gsmr_t)gsmi_parse_number(&str);
    if (0 != err) { /* open_state == 0 (success) */
        *is_error = (gsmr_t)err;
    }
}

static void gsmi_parse_socket_sta(uint8_t* is_ok, gsm_recv_t* rcv)
{
    /* Check if connection data received */
    if (rcv->len > 3) {
        uint8_t continueScan, processed = 0;
        if (!strncmp(rcv->data, "+QISTATE:", 9)) {
            processed = 1;
            gsmi_parse_socket_conn(rcv->data, &continueScan);
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

    /* Scan received strings which start with '+' */
    if (!strncmp(rcv->data, "+QNWINFO", 8)) {
        gsmi_parse_nwinfo(rcv->data);          /* Parse +QNWINFO response */
    } else if (!strncmp(rcv->data, "+QIACT", 6)) {
        gsmi_parse_qiact(is_ok, rcv->data);/* Update status */
    } else if (!strncmp(rcv->data, "+QIOPEN", 7)) {
        gsmi_parse_qiopen(is_error, rcv->data);/* Update socket opened or not */
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
}

/* instance platform */
const struct me_data bg95 = {
    .GSM_CMD_CGACT_SET_0      = "AT+CGACT=0,1" CRLF ,
    .GSM_CMD_CGACT_SET_1      = "AT+CGACT=1,1" CRLF ,
    .GSM_CMD_SOCKET_OPEN      = GSM_CMD_QIOPEN,
    .GSM_CMD_SOCKET_SEND      = GSM_CMD_QISEND,
    .GSM_CMD_SOCKET_CLOSE     = GSM_CMD_QICLOSE,
    .GSM_CMD_SOCKET_STA       = GSM_CMD_QISTATE,
    .creg_cgreg_skip_first    = 1,
    .high_baudrate            = 921600,
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
