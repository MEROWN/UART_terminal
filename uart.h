#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <termios.h>

/* BUFF SIZE for data received from RX */
#define BUF_SIZE 256

/* Map integer baud rate with constant BXXXXX*/
struct baud_mapping {
	uint32_t baud;
	speed_t speed_const;
};

/* Array of pairs {baud, const} */
static const struct baud_mapping baud_table[] = {
	{ 9600,    B9600   },
	{ 19200,   B19200  },
	{ 38400,   B38400  },
	{ 57600,   B57600  },
	{ 115200,  B115200 },
	{ 230400,  B230400 },
	{ 4000000, B4000000},
	/* Easy to expand */
};

#endif /* UART_H */
