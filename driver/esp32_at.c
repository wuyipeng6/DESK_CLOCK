#include "esp32_at.h"
#include "string.h"
#include "usart.h"
#include "usart2.h"
#include <stdio.h>

// Use existing USART2 buffer/flags from usart.c: USART2_RX_BUF, USART2_RX_STA
extern u8 USART2_RX_BUF[];
extern u16 USART2_RX_STA;

static void esp_clear_rx(void)
{
    // Clear receive index and zero the buffer.
    // This resets the circular buffer write index stored in the low bits
    // of `USART2_RX_STA` so the next received byte will be written at 0.
    USART2_RX_STA = 0;
    memset(USART2_RX_BUF, 0, USART_REC_LEN);
}

void ESP_Init(uint32_t baud)
{
    // Initialize USART2 for ESP communication
    // Call `uart2_init` to configure PA2/PA3 and enable USART2 Rx interrupts.
    uart2_init(baud);
    // Small delay to allow line settling and for ESP module to be ready
    Delay_ms(50);
}
int ESP_Reset(void)
{
#if defined(ESP_RST_GPIO_PORT) && defined(ESP_RST_PIN)
    GPIO_InitTypeDef GPIO_InitStructure;
    // Configure as push-pull output
    // Assumes RCC for the GPIO port is enabled by user/project
    GPIO_InitStructure.GPIO_Pin = ESP_RST_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(ESP_RST_GPIO_PORT, &GPIO_InitStructure);

    // Toggle reset
    GPIO_ResetBits(ESP_RST_GPIO_PORT, ESP_RST_PIN);
    Delay_ms(100);
    GPIO_SetBits(ESP_RST_GPIO_PORT, ESP_RST_PIN);
    Delay_ms(500);
    return 0;
#else
    return -1; // reset not supported (pin not defined)
#endif
}

int ESP_SendRaw(const uint8_t *data, size_t len)
{
    if (!data || len == 0) return -1;

    // debug: show raw send length
    //printf("ESP SendRaw %u bytes\r\n", (unsigned)len);

    for (size_t i = 0; i < len; ++i) {
        while ((USART2->SR & 0x40) == 0) ;
        USART2->DR = data[i];
    }
    return 0;
}

// Wait for response: either until CRLF-terminated buffer (usart sets bit15) or timeout
int ESP_WaitResponse(const char *expect, char *outbuf, size_t outlen, uint32_t timeout_ms)
{
    /*
     * Idle-based packet collection:
     * - The ISR appends bytes into `USART2_RX_BUF` and advances the index
     *   stored in `USART2_RX_STA` low bits.
     * - This function monitors the write index; when the index stops
     *   changing for `idle_threshold` milliseconds we assume the sender
     *   finished transmitting a packet. At that point the collected bytes
     *   are copied out and inspected for `expect`.
     * - If `expect` is not found, the buffer is cleared and waiting
     *   continues until overall `timeout_ms` expires.
     */
    uint32_t elapsed = 0;
    const uint32_t idle_threshold = 10; // ms of no new bytes to consider packet complete
    //idle_threshold值需要根据实际情况调整，太小可能会导致数据不完整，太大则会增加等待时间
    //将下面的printf注释去掉，可以查看输出情况，判断数据是否完整。
    uint32_t idle_count = 0;
    u16 last_idx = USART2_RX_STA & 0x3FFF;
    // reset output
    if (outbuf && outlen) outbuf[0] = '\0';

    while (elapsed < timeout_ms) {
        u16 cur_idx = USART2_RX_STA & 0x3FFF;
        if (cur_idx != last_idx) {
            // new data arrived — reset idle counter and remember position
            last_idx = cur_idx;
            idle_count = 0;
        } else {
            // no new data since last check — increment idle millisecond counter
            idle_count++;
            if (idle_count >= idle_threshold) {
                // consider buffer idle -> collect currently buffered bytes
                size_t n = (size_t)cur_idx;
                if (n >= outlen) n = outlen - 1;
                if (outbuf && outlen) {
                    if (n > 0) memcpy(outbuf, USART2_RX_BUF, n);
                    outbuf[n] = '\0';
                }
                // debug print of collected packet
                //printf("ESP Idle collect %u bytes: '%s'\r\n", (unsigned)n, outbuf?outbuf:"(null)");
                if (expect) {
                    if (strstr(outbuf, expect)) return 0; // expected content found
                    else {
                        // not found: clear buffer and continue waiting for next packet
                        USART2_RX_STA = 0;
                        memset(USART2_RX_BUF, 0, USART_REC_LEN);
                        last_idx = 0;
                        idle_count = 0;
                        // continue waiting until timeout
                    }
                } else {
                    // no specific expectation — treat collected data as success
                    return 0;
                }
            }
        }
        Delay_ms(1);
        elapsed++;
    }
    // timeout: copy partial buffer to outbuf and report
    if (outbuf && outlen) {
        u16 cur_idx = USART2_RX_STA & 0x3FFF;
        size_t n = (size_t)cur_idx;
        if (n >= outlen) n = outlen - 1;
        if (n > 0) memcpy(outbuf, USART2_RX_BUF, n);
        outbuf[n] = '\0';
    }
    printf("ESP WaitResponse timeout after %u ms, partial='%s'\r\n", (unsigned)timeout_ms, outbuf?outbuf:"(null)");
    return -1;
}

// Send AT command (adds CRLF) and wait for expected reply
int ESP_SendCmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    if (!cmd) return -1;
    esp_clear_rx();
    // Debug: print command sent to ESP
    //printf("ESP SendCmd: '%s' (expect='%s')\r\n", cmd, expect?expect:"(null)");

    // send command
    const char *p = cmd;
    while (*p) {
        while ((USART2->SR & 0x40) == 0) ;
        USART2->DR = (uint8_t)(*p++);
    }
    // append CRLF
    while ((USART2->SR & 0x40) == 0) ; USART2->DR = '\r';
    while ((USART2->SR & 0x40) == 0) ; USART2->DR = '\n';

    // wait for response
    char buf[USART_REC_LEN];
    int r = ESP_WaitResponse(expect, buf, sizeof(buf), timeout_ms);
    return r;
}
