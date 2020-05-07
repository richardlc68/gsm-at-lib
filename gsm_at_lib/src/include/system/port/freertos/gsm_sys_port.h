/***********************************************************************************************************************************
 *  PPPPPPP     AAAAAA    RRRRRRR     AAAAAA    DDDDDDD     OOOOOO    XX    XX
 *  PP    PP   AA    AA   RR    RR   AA    AA   DD    DD   OO    OO   XX    XX
 *  PP    PP   AA    AA   RR    RR   AA    AA   DD    DD   OO    OO    XX  XX
 *  PP    PP   AA    AA   RR    RR   AA    AA   DD    DD   OO    OO    XX  XX
 *  PPPPPPP    AAAAAAAA   RRRRRRR    AAAAAAAA   DD    DD   OO    OO     XXXX
 *  PP         AA    AA   RRRR       AA    AA   DD    DD   OO    OO     XXXX
 *  PP         AA    AA   RR RR      AA    AA   DD    DD   OO    OO    XX  XX
 *  PP         AA    AA   RR  RR     AA    AA   DD    DD   OO    OO    XX  XX
 *  PP         AA    AA   RR   RR    AA    AA   DD    DD   OO    OO   XX    XX
 *  PP         AA    AA   RR    RR   AA    AA   DDDDDDD     OOOOOO    XX    XX
 *
 *  S    E    C    U    R    I    T    Y       S    Y    S    T    E    M    S
 *
 ***********************************************************************************************************************************
 * All software (C) Copyright PARADOX Security Systems Bahamas Limited.
 **********************************************************************************************************************************/

/**
 * \file            gsm_sys_port.h
 * \brief           System dependent functions for CMSIS-OS based operating system
 */

/*
 * This file is part of GSM-AT.
 *
 * Author:          richardl@paradox.com
 * Version:         $_version_$
 */
#ifndef GSM_HDR_SYSTEM_PORT_H
#define GSM_HDR_SYSTEM_PORT_H

#include <stdint.h>
#include <stdlib.h>
#include "gsm_config.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if GSM_CFG_OS && !__DOXYGEN__

typedef SemaphoreHandle_t         gsm_sys_mutex_t;
typedef SemaphoreHandle_t         gsm_sys_sem_t;
typedef QueueHandle_t             gsm_sys_mbox_t;
typedef TaskHandle_t              gsm_sys_thread_t;
typedef UBaseType_t               gsm_sys_thread_prio_t;

#define osWaitForever               ((uint32_t)0xFFFFFFFF)
#define GSM_SYS_MUTEX_NULL          ((gsm_sys_mutex_t)0)
#define GSM_SYS_SEM_NULL            ((gsm_sys_sem_t)0)
#define GSM_SYS_MBOX_NULL           ((gsm_sys_mbox_t)0)
#define GSM_SYS_TIMEOUT             (osWaitForever)
#define GSM_SYS_THREAD_PRIO         (2)
#define GSM_SYS_THREAD_SS           (512)
#define osDelay                    vTaskDelay

#endif /* GSM_CFG_OS && !__DOXYGEN__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GSM_HDR_SYSTEM_PORT_H */
