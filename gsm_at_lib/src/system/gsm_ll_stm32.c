/**
 * \file            gsm_ll_stm32.c
 * \brief           Generic STM32 driver, included in various STM32 driver variants
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

/*
 * How it works
 *
 * On first call to \ref gsm_ll_init, new thread is created and processed in usart_ll_thread function.
 * USART is configured in RX DMA mode and any incoming bytes are processed inside thread function.
 * DMA and USART implement interrupt handlers to notify main thread about new data ready to send to upper layer.
 *
 * More about UART + RX DMA: https://github.com/MaJerle/stm32-usart-dma-rx-tx
 *
 * \ref GSM_CFG_INPUT_USE_PROCESS must be enabled in `gsm_config.h` to use this driver.
 */
#include "gsm/gsm.h"
#include "gsm/gsm_mem.h"
#include "gsm/gsm_input.h"
#include "system/gsm_ll.h"

#if !__DOXYGEN__

#if !GSM_CFG_INPUT_USE_PROCESS
#error "GSM_CFG_INPUT_USE_PROCESS must be enabled in `gsm_config.h` to use this driver."
#endif /* GSM_CFG_INPUT_USE_PROCESS */

#if !defined(GSM_USART_DMA_RX_BUFF_SIZE)
#define GSM_USART_DMA_RX_BUFF_SIZE      0x1000
#endif /* !defined(GSM_USART_DMA_RX_BUFF_SIZE) */

#if !defined(GSM_MEM_SIZE)
#define GSM_MEM_SIZE                    0x1000
#endif /* !defined(GSM_MEM_SIZE) */

#if !defined(GSM_USART_RDR_NAME)
#define GSM_USART_RDR_NAME              RDR
#endif /* !defined(GSM_USART_RDR_NAME) */

/* USART memory */
static uint8_t      usart_mem[GSM_USART_DMA_RX_BUFF_SIZE];
static uint8_t      is_running, initialized;
static size_t       old_pos;

/* USART thread */
static void usart_ll_thread(void* arg);
static osThreadId_t usart_ll_thread_id;

/* Message queue */
static osMessageQueueId_t usart_ll_mbox_id;

/**
 * \brief           USART data processing
 */
static void
usart_ll_thread(void* arg) {
    size_t pos;

    GSM_UNUSED(arg);

    while (1) {
        void* d;
        /* Wait for the event message from DMA or USART */
        osMessageQueueGet(usart_ll_mbox_id, &d, NULL, osWaitForever);

        /* Read data */
#if defined(GSM_USART_DMA_RX_STREAM)
        pos = sizeof(usart_mem) - LL_DMA_GetDataLength(GSM_USART_DMA, GSM_USART_DMA_RX_STREAM);
#else
        pos = sizeof(usart_mem) - LL_DMA_GetDataLength(GSM_USART_DMA, GSM_USART_DMA_RX_CH);
#endif /* defined(GSM_USART_DMA_RX_STREAM) */
        if (pos != old_pos && is_running) {
            if (pos > old_pos) {
                gsm_input_process(&usart_mem[old_pos], pos - old_pos);
            } else {
                gsm_input_process(&usart_mem[old_pos], sizeof(usart_mem) - old_pos);
                if (pos > 0) {
                    gsm_input_process(&usart_mem[0], pos);
                }
            }
            old_pos = pos;
            if (old_pos == sizeof(usart_mem)) {
                old_pos = 0;
            }
        }
    }
}

/**
 * \brief           Configure UART using DMA for receive in double buffer mode and IDLE line detection
 */
gsmr_t
configure_uart(uint32_t baudrate) {
    static LL_USART_InitTypeDef usart_init;
    static LL_DMA_InitTypeDef dma_init;
    LL_GPIO_InitTypeDef gpio_init;

    if (!initialized) {
        /* Enable peripheral clocks */
        GSM_USART_CLK;
        GSM_USART_DMA_CLK;
        GSM_USART_TX_PORT_CLK;
        GSM_USART_RX_PORT_CLK;

#if defined(GSM_RESET_PIN)
        GSM_RESET_PORT_CLK;
#endif /* defined(GSM_RESET_PIN) */

        /* Global pin configuration */
        LL_GPIO_StructInit(&gpio_init);
        gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpio_init.Pull = LL_GPIO_PULL_UP;
        gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Mode = LL_GPIO_MODE_OUTPUT;

#if defined(GSM_RESET_PIN)
        /* Configure RESET pin */
        gpio_init.Pin = GSM_RESET_PIN;
        LL_GPIO_Init(GSM_RESET_PORT, &gpio_init);
#endif /* defined(GSM_RESET_PIN) */

        /* Configure USART pins */
        gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;

        /* TX PIN */
        gpio_init.Alternate = GSM_USART_TX_PIN_AF;
        gpio_init.Pin = GSM_USART_TX_PIN;
        LL_GPIO_Init(GSM_USART_TX_PORT, &gpio_init);

        /* RX PIN */
        gpio_init.Alternate = GSM_USART_RX_PIN_AF;
        gpio_init.Pin = GSM_USART_RX_PIN;
        LL_GPIO_Init(GSM_USART_RX_PORT, &gpio_init);

        /* Configure UART */
        LL_USART_DeInit(GSM_USART);
        LL_USART_StructInit(&usart_init);
        usart_init.BaudRate = baudrate;
        usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
        usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
        usart_init.OverSampling = LL_USART_OVERSAMPLING_16;
        usart_init.Parity = LL_USART_PARITY_NONE;
        usart_init.StopBits = LL_USART_STOPBITS_1;
        usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
        LL_USART_Init(GSM_USART, &usart_init);

        /* Enable USART interrupts and DMA request */
        LL_USART_EnableIT_IDLE(GSM_USART);
        LL_USART_EnableIT_PE(GSM_USART);
        LL_USART_EnableIT_ERROR(GSM_USART);
        LL_USART_EnableDMAReq_RX(GSM_USART);

        /* Enable USART interrupts */
        NVIC_SetPriority(GSM_USART_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x07, 0x00));
        NVIC_EnableIRQ(GSM_USART_IRQ);

        /* Configure DMA */
        is_running = 0;
#if defined(GSM_USART_DMA_RX_STREAM)
        LL_DMA_DeInit(GSM_USART_DMA, GSM_USART_DMA_RX_STREAM);
        dma_init.Channel = GSM_USART_DMA_RX_CH;
#else
        LL_DMA_DeInit(GSM_USART_DMA, GSM_USART_DMA_RX_CH);
        dma_init.PeriphRequest = GSM_USART_DMA_RX_REQ_NUM;
#endif /* defined(GSM_USART_DMA_RX_STREAM) */
        dma_init.PeriphOrM2MSrcAddress = (uint32_t)&GSM_USART->GSM_USART_RDR_NAME;
        dma_init.MemoryOrM2MDstAddress = (uint32_t)usart_mem;
        dma_init.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
        dma_init.Mode = LL_DMA_MODE_CIRCULAR;
        dma_init.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
        dma_init.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
        dma_init.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
        dma_init.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
        dma_init.NbData = sizeof(usart_mem);
        dma_init.Priority = LL_DMA_PRIORITY_MEDIUM;
#if defined(GSM_USART_DMA_RX_STREAM)
        LL_DMA_Init(GSM_USART_DMA, GSM_USART_DMA_RX_STREAM, &dma_init);
#else
        LL_DMA_Init(GSM_USART_DMA, GSM_USART_DMA_RX_CH, &dma_init);
#endif /* defined(GSM_USART_DMA_RX_STREAM) */

        /* Enable DMA interrupts */
#if defined(GSM_USART_DMA_RX_STREAM)
        LL_DMA_EnableIT_HT(GSM_USART_DMA, GSM_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_TC(GSM_USART_DMA, GSM_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_TE(GSM_USART_DMA, GSM_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_FE(GSM_USART_DMA, GSM_USART_DMA_RX_STREAM);
        LL_DMA_EnableIT_DME(GSM_USART_DMA, GSM_USART_DMA_RX_STREAM);
#else
        LL_DMA_EnableIT_HT(GSM_USART_DMA, GSM_USART_DMA_RX_CH);
        LL_DMA_EnableIT_TC(GSM_USART_DMA, GSM_USART_DMA_RX_CH);
        LL_DMA_EnableIT_TE(GSM_USART_DMA, GSM_USART_DMA_RX_CH);
#endif /* defined(GSM_USART_DMA_RX_STREAM) */

        /* Enable DMA interrupts */
        NVIC_SetPriority(GSM_USART_DMA_RX_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0x07, 0x00));
        NVIC_EnableIRQ(GSM_USART_DMA_RX_IRQ);

        old_pos = 0;
        is_running = 1;

        /* Start DMA and USART */
#if defined(GSM_USART_DMA_RX_STREAM)
        LL_DMA_EnableStream(GSM_USART_DMA, GSM_USART_DMA_RX_STREAM);
#else
        LL_DMA_EnableChannel(GSM_USART_DMA, GSM_USART_DMA_RX_CH);
#endif /* defined(GSM_USART_DMA_RX_STREAM) */
        LL_USART_Enable(GSM_USART);
    } else {
        osDelay(10);
        LL_USART_Disable(GSM_USART);
        usart_init.BaudRate = baudrate;
        LL_USART_Init(GSM_USART, &usart_init);
        LL_USART_Enable(GSM_USART);
    }

    /* Create mbox and start thread */
    if (usart_ll_mbox_id == NULL) {
        usart_ll_mbox_id = osMessageQueueNew(10, sizeof(void *), NULL);
    }
    if (usart_ll_thread_id == NULL) {
        const osThreadAttr_t attr = {
            .stack_size = 1024
        };
        usart_ll_thread_id = osThreadNew(usart_ll_thread, usart_ll_mbox_id, &attr);
    }
    return gsmOK;
}

#if defined(GSM_RESET_PIN)
/**
 * \brief           Hardware reset callback
 */
static uint8_t
reset_device(uint8_t state) {
    if (state) {                                /* Activate reset line */
        LL_GPIO_ResetOutputPin(GSM_RESET_PORT, GSM_RESET_PIN);
    } else {
        LL_GPIO_SetOutputPin(GSM_RESET_PORT, GSM_RESET_PIN);
    }
    return 1;
}
#endif /* defined(GSM_RESET_PIN) */

/**
 * \brief           Send data to GSM device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static size_t
send_data(const void* data, size_t len) {
    const uint8_t* d = data;

    for (size_t i = 0; i < len; ++i, ++d) {
        LL_USART_TransmitData8(GSM_USART, *d);
        while (!LL_USART_IsActiveFlag_TXE(GSM_USART)) {}
    }
    return len;
}

/**
 * \brief           Callback function called from initialization process
 * \note            This function may be called multiple times if AT baudrate is changed from application
 * \param[in,out]   ll: Pointer to \ref gsm_ll_t structure to fill data for communication functions
 * \param[in]       baudrate: Baudrate to use on AT port
 * \return          Member of \ref gsmr_t enumeration
 */
gsmr_t
gsm_ll_init(gsm_ll_t* ll) {
#if !GSM_CFG_MEM_CUSTOM
    static uint8_t memory[GSM_MEM_SIZE];
    gsm_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };

    if (!initialized) {
        gsm_mem_assignmemory(mem_regions, GSM_ARRAYSIZE(mem_regions));  /* Assign memory for allocations */
    }
#endif /* !GSM_CFG_MEM_CUSTOM */

    if (!initialized) {
        ll->send_fn = send_data;                /* Set callback function to send data */
#if defined(GSM_RESET_PIN)
        ll->reset_fn = reset_device;            /* Set callback for hardware reset */
#endif /* defined(GSM_RESET_PIN) */
    }

    configure_uart(ll->uart.baudrate);          /* Initialize UART for communication */
    initialized = 1;
    return gsmOK;
}

/**
 * \brief           Callback function to de-init low-level communication part
 * \param[in,out]   ll: Pointer to \ref gsm_ll_t structure to fill data for communication functions
 * \return          \ref gsmOK on success, member of \ref gsmr_t enumeration otherwise
 */
gsmr_t
gsm_ll_deinit(gsm_ll_t* ll) {
    if (usart_ll_mbox_id != NULL) {
        osMessageQueueId_t tmp = usart_ll_mbox_id;
        usart_ll_mbox_id = NULL;
        osMessageQueueDelete(tmp);
    }
    if (usart_ll_thread_id != NULL) {
        osThreadId_t tmp = usart_ll_thread_id;
        usart_ll_thread_id = NULL;
        osThreadTerminate(tmp);
    }
    initialized = 0;
    GSM_UNUSED(ll);
    return gsmOK;
}

/**
 * \brief           UART global interrupt handler
 */
void
GSM_USART_IRQHANDLER(void) {
    LL_USART_ClearFlag_IDLE(GSM_USART);
    LL_USART_ClearFlag_PE(GSM_USART);
    LL_USART_ClearFlag_FE(GSM_USART);
    LL_USART_ClearFlag_ORE(GSM_USART);
    LL_USART_ClearFlag_NE(GSM_USART);

    if (usart_ll_mbox_id != NULL) {
        void* d = (void *)1;
        osMessageQueuePut(usart_ll_mbox_id, &d, 0, 0);
    }
}

/**
 * \brief           UART DMA stream/channel handler
 */
void
GSM_USART_DMA_RX_IRQHANDLER(void) {
    GSM_USART_DMA_RX_CLEAR_TC;
    GSM_USART_DMA_RX_CLEAR_HT;

    if (usart_ll_mbox_id != NULL) {
        void* d = (void *)1;
        osMessageQueuePut(usart_ll_mbox_id, &d, 0, 0);
    }
}

#endif /* !__DOXYGEN__ */
