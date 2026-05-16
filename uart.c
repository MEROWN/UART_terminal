#define _GNU_SOURCE
#include "uart.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

/**
 * This function looks through table of pairs {uint32_t baud_rate, speed_t baud_const}
 * Return baud_const when meets pair with same baud_const
 * Return B0 if none found
 */
speed_t get_baud(uint32_t baud)
{
	size_t size = sizeof(baud_table) / sizeof(struct baud_mapping);
	for (size_t i = 0; i < size; i++) {
		if (baud_table[i].baud == baud)
			return baud_table[i].speed_const;
	}
	return B0;
}

int
configure_uart(int fd, uint32_t baud, uint32_t data, uint32_t stop, char parity)
{
	struct termios tty;
	
	/* getting file descriptor */
	if (tcgetattr(fd, &tty) != 0) {
		perror("tcgetattr");
		return -1;
	}

	/* setting speed */
	speed_t speed = get_baud(baud);
	if (speed == B0) {
		fprintf(stderr, "Invalid baud rate\n");
		return -1;
	}
	cfsetispeed(&tty, speed);
	cfsetospeed(&tty, speed);

	/* setting data bits */
	tty.c_cflag &= ~CSIZE; /* Nullifying old settings */
	switch (data) {
		case 5:
			tty.c_cflag |= CS5;
			break;
		case 6:
			tty.c_cflag |= CS6;
			break;
		case 7:
			tty.c_cflag |= CS7;
			break;
		case 8:
			tty.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr, "Invalid data bits\n");
			return -1;
	}

	/* setting parity */
	switch (parity) {
		case 'N':
		case 'n':
			tty.c_cflag &= ~PARENB;
			break;
		case 'E':
		case 'e':
			tty.c_cflag |= PARENB;
			tty.c_cflag &= ~PARODD;
			break;
		case 'O':
		case 'o':
			tty.c_cflag |= PARENB;
			tty.c_cflag |= PARODD;
			break;
		default:
			fprintf(stderr, "Invalid parity mode\n");
			return -1;
	}

	/* setting stop bits */
	if (stop == 2)
		tty.c_cflag |= CSTOPB;
	else
		tty.c_cflag &= ~CSTOPB;
	
	/* Recive enable and ignore DTR/RTS/CTS */
	tty.c_cflag |= (CLOCAL | CREAD);

	/* RAW MODE */
	
	/**
	 * Disable canonic mode
	 * Disable echoing symbols to not cycle data transmition RX -> TX -> RX
	 * Disable bacspace and terminal inputs, e.g. ctrl-c
	 */
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	
	/**
	 * first 4 disable special bytes, like XON and XOFF
	 * Disable ignore and interrupt send if UART is in BREAK state
	 * Disable parity mark and 8 bit stripping
	 * Disable newline charaters translation, e.g. \r (0x0D) -> \n (0x0A)
	 */
	tty.c_iflag &= ~(IXON   | IXOFF  | IXANY  |
			 IGNBRK | BRKINT |
			 PARMRK | ISTRIP |
			 INLCR  | IGNCR  | ICRNL);
	
	/**
	 * Disable output post-processing to avoid data alteration
	 */
	tty.c_oflag &= ~OPOST;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		perror("tcsetattr");
		return -1;
	}
	return 0;
}

int main(int argc, char** argv)
{
	int opt;
	/* standard read values */
	const char *device = NULL;	/* -a(ddres)    */
	uint32_t baud_rate = 115200;	/* -b(aud rate) */
	char parity = 'N';		/* -p(arity)    */
	uint32_t stop_bits = 1;		/* -s(top bits) */
	uint32_t data_bits = 8; 	/* -d(ata bits) */
	struct pollfd pfd;
	/*
	 * parsing arguments
	 * getopt returns pointer to first character of argument
	 */
	while ((opt = getopt(argc, argv, "a:b:p:s:d:")) != -1) {
		switch (opt) {
			case 'a':
				device = optarg; 
				break;
			case 'b':
				baud_rate = atoi(optarg); 
				break;
			case 'p':
				parity = optarg[0]; 
				break;
			case 's':
				stop_bits = atoi(optarg);
				break;
			case 'd':
				data_bits = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Usage: ./uart -a device_path -b baud_rate -p parity -s stop_bits -d data_bits\n");
				exit(EXIT_FAILURE);
		}
	}
	if (device == NULL) {
        	fprintf(stderr, "Error: Device address (-a) is required.\n");
        	exit(EXIT_FAILURE);
	}

	int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		perror("Error opening UART device");
		exit(EXIT_FAILURE);
	}
	
	if (configure_uart(fd, baud_rate, data_bits, stop_bits, parity) < 0) {
		close(fd);
		exit(EXIT_FAILURE);
	}
	
	/* Test message */
	const char *test_msg = "Hello, UART\n";
	ssize_t bytes_written = write(fd, test_msg, strlen(test_msg));
	if (bytes_written < 0) {
		perror("Write failed");
		close(fd);
		exit(EXIT_FAILURE);
	}

	/* setting up pollfd struct */
	pfd.fd = fd;
	pfd.events = POLLIN;
	
	/* Wait for incoming data with a 3-second timeout (3000 ms) */
	int ret = poll(&pfd, 1, 3000);

	if (ret < 0) {
		perror("poll failed");
		close(fd);
		exit(EXIT_FAILURE);
	} else if (ret == 0) {
		printf("Timeout reached! No data received\n");
	} else {
		/* Check if device is connected and is not faulty */
		if (pfd.revents & (POLLERR | POLLHUP)) {
				fprintf(stderr, "Device error or disconected\n");
				close(fd);
				exit(EXIT_FAILURE);
			}

		/* Check if data is ready to be read */
		if (pfd.revents & POLLIN) {
			char rx_buf[BUF_SIZE];
			ssize_t bytes_read = read(fd, rx_buf, sizeof(rx_buf) - 1);
			if (bytes_read < 0) {
				perror("read failed");
				close(fd);
				exit(EXIT_FAILURE);
			} else if (bytes_read == 0) {
				fprintf(stderr, "Connection closed by device\n");
				close(fd);
				exit(EXIT_FAILURE);
			} else {
				rx_buf[bytes_read] = '\0'; /* Null-terminate string safely */
				printf("Received %zd bytes: %s\n", bytes_read, rx_buf);
			}
		}
	}

	close(fd);
	return 0;
}
