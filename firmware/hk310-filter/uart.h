#ifndef __UART_H
#define __UART_H

extern void Init_UART(void);
extern uint8_t UART_read_byte(void);
extern void UART_send(uint8_t);
extern void UART_send_uint(uint16_t);
extern void UART_send_uchar(uint8_t);
extern void UART_send_binary_uint(uint16_t);
extern uint8_t UART_receive_byte_pending(void);

#endif /* __UART_H */
