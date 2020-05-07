/**
 * \file            gsm_sys_cmsis_os.c
 * \brief           System dependent functions for CMSIS-OS based operating system
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
#include "system/gsm_sys.h"
#include "gsm_sys_port.h"

#if !__DOXYGEN__

static SemaphoreHandle_t  sys_mutex;

uint8_t
gsm_sys_init(void) {
    gsm_sys_mutex_create(&sys_mutex);
    return pdTRUE;
}

uint32_t
gsm_sys_now(void) {
    return xTaskGetTickCount();
}

uint8_t
gsm_sys_protect(void) {
    gsm_sys_mutex_lock(&sys_mutex);
    return pdTRUE;
}

uint8_t
gsm_sys_unprotect(void) {
    gsm_sys_mutex_unlock(&sys_mutex);
    return pdTRUE;
}

uint8_t
gsm_sys_mutex_create(gsm_sys_mutex_t* p) {
    *p = xSemaphoreCreateRecursiveMutex();
    return *p != NULL;
}

uint8_t
gsm_sys_mutex_delete(gsm_sys_mutex_t* p) {
    vSemaphoreDelete(*p);
    *p=NULL;
    return pdTRUE;
}

uint8_t
gsm_sys_mutex_lock(gsm_sys_mutex_t* p) {
    return xSemaphoreTakeRecursive(*p, osWaitForever) == pdTRUE;
}

uint8_t
gsm_sys_mutex_unlock(gsm_sys_mutex_t* p) {
    return xSemaphoreGiveRecursive(*p) == pdTRUE;
}

uint8_t
gsm_sys_mutex_isvalid(gsm_sys_mutex_t* p) {
    return p != NULL && *p != NULL;
}

uint8_t
gsm_sys_mutex_invalid(gsm_sys_mutex_t* p) {
    *p = GSM_SYS_MUTEX_NULL;
    return 1;
}

uint8_t
gsm_sys_sem_create(gsm_sys_sem_t* p, uint8_t cnt) {
    if(cnt>0) {
        *p = xSemaphoreCreateCounting(cnt,0);
    }
    else {
        *p = xSemaphoreCreateBinary();
    }
    return pdTRUE;
}

uint8_t
gsm_sys_sem_delete(gsm_sys_sem_t* p) {
    vSemaphoreDelete(*p);
    *p=NULL;
    return pdTRUE;
}

uint32_t
gsm_sys_sem_wait(gsm_sys_sem_t* p, uint32_t timeout) {
    if(pdTRUE  == xSemaphoreTake(*p,timeout)) return 0;
    return GSM_SYS_TIMEOUT;
}

uint8_t
gsm_sys_sem_release(gsm_sys_sem_t* p) {
    return xSemaphoreGiveRecursive(*p) == pdTRUE;
}

uint8_t
gsm_sys_sem_isvalid(gsm_sys_sem_t* p) {
    return p != NULL && *p != NULL;
}

uint8_t
gsm_sys_sem_invalid(gsm_sys_sem_t* p) {
    *p = GSM_SYS_SEM_NULL;
    return 1;
}

uint8_t
gsm_sys_mbox_create(gsm_sys_mbox_t* b, size_t size) {
    return (*b = xQueueCreate(size,sizeof(void *))) != NULL;
}

uint8_t
gsm_sys_mbox_delete(gsm_sys_mbox_t* b) {
    if (uxQueueMessagesWaiting(*b) > 0 ) {
        return 0;
    }
    vQueueDelete(*b);
    *b=0;
    return pdTRUE;
}

uint32_t
gsm_sys_mbox_put(gsm_sys_mbox_t* b, void* m) {
    return xQueueSend(*b, &m, osWaitForever) == pdTRUE ? 0 : GSM_SYS_TIMEOUT;
}

uint32_t
gsm_sys_mbox_get(gsm_sys_mbox_t* b, void** m, uint32_t timeout) {
    return xQueueReceive(*b, m, timeout) == pdTRUE ? 0 : GSM_SYS_TIMEOUT;;
}

uint8_t
gsm_sys_mbox_putnow(gsm_sys_mbox_t* b, void* m) {
    return xQueueSend(*b, &m, 0) == pdTRUE;
}

uint8_t
gsm_sys_mbox_getnow(gsm_sys_mbox_t* b, void** m) {
    return xQueueReceive(*b, m, 0) == pdTRUE;
}

uint8_t
gsm_sys_mbox_isvalid(gsm_sys_mbox_t* b) {
    return b != NULL && *b != NULL;
}

uint8_t
gsm_sys_mbox_invalid(gsm_sys_mbox_t* b) {
    *b = GSM_SYS_MBOX_NULL;
    return 1;
}

uint8_t
gsm_sys_thread_create(gsm_sys_thread_t* t, const char* name, gsm_sys_thread_fn thread_func, void* const arg, size_t stack_size, gsm_sys_thread_prio_t prio) {
    gsm_sys_thread_t id;
    if(xTaskCreate((TaskFunction_t)thread_func, 
                   name, 
                   stack_size > 0 ? stack_size : GSM_SYS_THREAD_SS,
                   arg,
                   prio,
                   &id) != pdPASS ) return pdFALSE;
    *t = id;
    return pdTRUE;
}

uint8_t
gsm_sys_thread_terminate(gsm_sys_thread_t* t) {
    if (t != NULL) {
        vTaskDelete(*t);
        *t=NULL;
    }
    return 1;
}

uint8_t
gsm_sys_thread_yield(void) {
    //osThreadYield();
    return 1;
}

#endif /* !__DOXYGEN__ */
