/*
 * Library to drive esp2866 serial to wifi board.
 */

/*
 * Copyright 2015 Martyn Welch <martyn@welchs.me.uk>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uart.h"
#include "esp8266.h"

char expect_buf[EXPECT_BUF_SIZE];

int8_t expect(char *pattern)
{
	int16_t size;
	int16_t overflow;
	char *write_ptr;
	char *cmp_ptr;

	/* Sort out "before" buffer */
	write_ptr = expect_buf;
	cmp_ptr = expect_buf;
	memset(expect_buf, '\0', EXPECT_BUF_SIZE);

	/* Create match buffer */
	size = strlen(pattern);

	/*
	 * Protect against overflowing "before" buffer - instead, enable return
	 * to user as a kind of timeout.
	 */
	overflow = EXPECT_BUF_SIZE - size;

	while (strncmp(pattern, cmp_ptr, size) != 0) {

		if (overflow == 0) {
			return -1;
		}

		overflow--;

		*write_ptr = getc(stdin);
		write_ptr++;

		while((write_ptr - cmp_ptr) > size)
			cmp_ptr++;
	}

	return 0;
}

int8_t esp8266_init(void)
{
	int8_t retval;
	char *match;

	printf("AT+RST\r\n");
	retval = expect("www.ai-thinker.com]\r\n");

	printf("ATE0\r\n");

	retval = expect("\r\nOK\r\n");
	if (retval != 0)
		return retval;


	printf("AT+CWMODE?\r\n");

	retval = expect("\r\nOK\r\n");
	if (retval != 0)
		return retval;

	match = strstr(expect_buf, "+CWMODE:");
	if (match == NULL)
		return -1;

	if (match[8] != '1') {
		printf("AT+CWMODE=1\r\n");

		retval = expect("\r\nOK\r\n");
		if (retval != 0)
			return retval;
	}

	printf("AT+CIPMUX=0\r\n");

	retval = expect("\r\nOK\r\n");
	if (retval != 0)
		return retval;

	return 0;
}

int8_t esp8266_network(char *ssid, char *passwd)
{
	int8_t retval;
#if 0
	char *match;

	printf("AT+CWJAP?\r\n");

	retval = expect("\r\nOK\r\n");
	if (retval != 0)
		return retval;

	match = strstr(expect_buf, "+CWJAP:");
	if (match == NULL)
		return -1;

	if (strstr(match + 7, ssid) == NULL) {
#endif
		printf("AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, passwd);

		retval = expect("\r\nOK\r\n");
		if (retval != 0)
			return retval;
#if 0
	}
#endif

	return 0;
}

int8_t esp8266_http_get(char *ip, char *get_str)
{
	int8_t retval;
	int16_t len;
	char *msg;

	printf("AT+CIPSTART=\"TCP\",\"%s\",80\r\n", ip);

	retval = expect("\r\nOK\r\n");
	if (retval != 0)
		return retval;

	retval = expect("Linked\r\n");
	if (retval != 0)
		return retval;

	len = 60 +  strlen(ip) + strlen(get_str);

	msg = malloc(len);

	snprintf(msg, len, "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: ESP8266/1.0\r\n\r\n", get_str, ip);

	len = strlen(msg);

	printf("AT+CIPSEND=%d\r\n", len);
	retval = expect("> ");
	if (retval != 0)
		return retval;

	printf(msg);
	retval = expect("\r\nSEND OK\r\n");
	if (retval != 0)
		return retval;

	retval = expect("\r\nOK\r\n");
	if (retval != 0)
		return retval;

	printf("AT+CIPCLOSE\r\n");
	retval = expect("\r\nOK\r\n");
	if (retval != 0)
		return retval;

	retval = expect("Unlink\r\n");
	if (retval != 0)
		return retval;

	return 0;
}

