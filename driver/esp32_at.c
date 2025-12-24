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
// ------------------------------------------------------------------
// NOTE: 保留原有的 ESP_WaitResponse 实现可供需要按包闲置检测的调用使用。
// 为更可靠地与 AT 命令配对，这里新增一个按字节增量匹配的实现，
// 它在发送命令前不会清空接收缓冲，而是把内部读取位置设为当前
// 写入计数，从而仅处理后续到来的字节。
// ------------------------------------------------------------------

// Incremental byte-wise matcher helpers (non-destructive to RX buffer)
// Reads new bytes appended by the ISR (which increments USART2_RX_STA low bits)
// and performs simple pattern matching for OK / ERROR.
static volatile uint16_t esp_rx_read_pos = 0;

// Try to pop one new byte from hardware RX buffer. Returns 1 if a byte
// was returned, 0 otherwise.
static int esp_hw_pop_byte(uint8_t *out)
{
    u16 write_cnt = USART2_RX_STA & 0x3FFF;
    if (esp_rx_read_pos < write_cnt) {
        *out = USART2_RX_BUF[esp_rx_read_pos++];
        return 1;
    }
    return 0;
}

typedef enum {ESP_RES_NONE = 0, ESP_RES_OK, ESP_RES_ERROR} esp_res_t;

typedef struct { const char *pat; uint8_t len; uint8_t idx; } esp_pat_t;
static esp_pat_t esp_pat_ok = {"OK\r\n", 4, 0};
static esp_pat_t esp_pat_error = {"ERROR", 5, 0};

// Feed one byte into simple matchers; returns ESP_RES_OK / ESP_RES_ERROR / ESP_RES_NONE
static esp_res_t esp_feed_byte(uint8_t b)
{
    // OK matcher
    if (b == (uint8_t)esp_pat_ok.pat[esp_pat_ok.idx]) {
        esp_pat_ok.idx++;
        if (esp_pat_ok.idx == esp_pat_ok.len) {
            esp_pat_ok.idx = 0;
            esp_pat_error.idx = 0;
            return ESP_RES_OK;
        }
    } else {
        esp_pat_ok.idx = (b == (uint8_t)esp_pat_ok.pat[0]) ? 1 : 0;
    }

    // ERROR matcher
    if (b == (uint8_t)esp_pat_error.pat[esp_pat_error.idx]) {
        esp_pat_error.idx++;
        if (esp_pat_error.idx == esp_pat_error.len) {
            esp_pat_error.idx = 0;
            esp_pat_ok.idx = 0;
            return ESP_RES_ERROR;
        }
    } else {
        esp_pat_error.idx = (b == (uint8_t)esp_pat_error.pat[0]) ? 1 : 0;
    }

    return ESP_RES_NONE;
}

// Send command and wait for OK / ERROR using incremental matching.
// This does NOT clear the global RX buffer; it only advances an internal
// read pointer to start reading bytes that arrive after the call.
// Returns 0 = OK, -1 = ERROR, -2 = TIMEOUT
int ESP_SendCmd_NoClear(const char *cmd, uint32_t timeout_ms)
{
    if (!cmd) return -2;

    // initialize read pointer to current write count to ignore previous bytes
    esp_rx_read_pos = USART2_RX_STA & 0x3FFF;

    // reset pattern indices
    esp_pat_ok.idx = 0;
    esp_pat_error.idx = 0;

    // send command + CRLF
    const char *p = cmd;
    while (*p) {
        while ((USART2->SR & 0x40) == 0) ;
        USART2->DR = (uint8_t)(*p++);
    }
    while ((USART2->SR & 0x40) == 0) ; USART2->DR = '\r';
    while ((USART2->SR & 0x40) == 0) ; USART2->DR = '\n';

    uint32_t elapsed = 0;
    uint8_t ch;
    while (elapsed < timeout_ms) {
        if (esp_hw_pop_byte(&ch)) {
            esp_res_t r = esp_feed_byte(ch);
            if (r == ESP_RES_OK) {
                printf("ESP_SendCmd_NoClear OK: cmd='%s'\r\n", cmd);
                return 0;
            }
            if (r == ESP_RES_ERROR) {
                printf("ESP_SendCmd_NoClear ERROR: cmd='%s'\r\n", cmd);
                return -1;
            }
            // otherwise continue
        } else {
            Delay_ms(1);
            elapsed++;
        }
    }
    printf("ESP_SendCmd_NoClear TIMEOUT: cmd='%s' after %u ms\r\n", cmd, (unsigned)timeout_ms);
    return -2; // timeout
}

// Replace the original ESP_SendCmd to use the incremental matcher by default.
int ESP_SendCmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    if (!cmd) return -1;
    // Prefer incremental OK/ERROR matching. If caller provided a specific
    // `expect` string that is not handled here, fall back to older idle-based
    // collector by calling ESP_WaitResponse if needed.
    (void)expect;
    int r = ESP_SendCmd_NoClear(cmd, timeout_ms);
    if (r == 0) return 0;    // OK
    if (r == -1) return -1;  // ERROR
    return -1; // timeout or other treated as error
}

// Simple wait-for-expect implementation (reads new bytes after call).
int ESP_WaitResponse(const char *expect, char *outbuf, size_t outlen, uint32_t timeout_ms)
{
    if (outbuf && outlen) outbuf[0] = '\0';
    if (!expect) return -1;

    // start reading from current write position
    esp_rx_read_pos = USART2_RX_STA & 0x3FFF;

    uint32_t elapsed = 0;
    size_t pos = 0;
    uint8_t ch;
    while (elapsed < timeout_ms) {
        if (esp_hw_pop_byte(&ch)) {
            if (outbuf && outlen) {
                if (pos + 1 < outlen) {
                    outbuf[pos++] = ch;
                    outbuf[pos] = '\0';
                } else {
                    // buffer full: shift left by one to keep trailing data
                    if (outlen > 1) {
                        memmove(outbuf, outbuf + 1, outlen - 2);
                        outbuf[outlen - 2] = ch;
                        outbuf[outlen - 1] = '\0';
                    }
                }
                if (strstr(outbuf, expect)) return 0;
            }
        } else {
            Delay_ms(1);
            elapsed++;
        }
    }
    printf("ESP_WaitResponse TIMEOUT: expect='%s' after %u ms\r\n", expect, (unsigned)timeout_ms);
    return -1;
}

// Connect to AP using AT commands. Returns 0 on success, -1 on failure.
int ESP_ConnectAP(const char *ssid, const char *pwd, uint32_t timeout_ms)
{
    if (!ssid) return -1;

    // Ensure station mode
    ESP_SendCmd("AT+CWMODE=1", NULL, 2000);

    char cmd[256];
    if (pwd && pwd[0]) {
        snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"\"", ssid);
    }

    uint32_t to = (timeout_ms == 0) ? 20000 : timeout_ms;
    int r = ESP_SendCmd(cmd, NULL, to);
    if (r == 0) {
        return 0;
    }
    printf("ESP_ConnectAP: join failed for SSID='%s'\r\n", ssid);
    return -1;
}
