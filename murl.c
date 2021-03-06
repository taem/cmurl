/*
 * Murl.kz API implementation
 *
 * Copyright (C) 2011 Timur Birsh <taem@linukz.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Porting credits:
 *   Windows: resurtm <resurtm@gmail.com>
 */

#ifdef _MSC_VER
  #pragma comment(lib, "ws2_32.lib")
  #pragma comment(lib, "wsock32.lib")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef _MSC_VER
  #define ssize_t int
#else
  #include <unistd.h>
#endif
#include <string.h>
#include <errno.h>

#ifdef _WIN32
  #define _WIN32_WINNT 0x501
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
#endif

#include "murl.h"

#define API_HOST		"api.murl.kz"
#define API_HANDLER_BASIC	"/basic?format=rfc822&api_key=%s&url=%s"

/* Returns a url-encoded version of str */
extern char *url_encode(char *str); /* urlcode.c */
static char *murl_sprintf(const char *fmt, ...);
static int murl_req(char *req, const char *hostname, struct murl_response *res,
		murl_http_request cb);

/*
 * Get short link for long URL
 */
const int murlificate(const char *api_key, const char *url,
		struct murl_response *res, murl_http_request cb)
{
	char *urlenc, *api;
	int ret = -MURL_ERR_MEM;

	if ((urlenc = url_encode((char *) url)) == NULL)
		return ret;

	/* Build API string */
	if ((api = murl_sprintf(API_HANDLER_BASIC, api_key, urlenc)) != NULL) {
		ret = murl_req(api, API_HOST, res, cb);
		free(api);
	}

	free(urlenc);
	return ret;
}

/* The length of the correct server */
/* reply should not exceed this value. */
#define BUFSIZE			512

static int http_request(const char *hostname, const char *req, char *reply);
static int parse_reply(char *reply, struct murl_response *res);

/*
 * Send request to the API server and parse its reply
 */
static int murl_req(char *req, const char *hostname, struct murl_response *res,
		murl_http_request cb)
{
	char *reply;
	int ret = -MURL_ERR_MEM;

	/* Allocate memory for the server reply */
	if ((reply = malloc(BUFSIZE)) == NULL)
		return ret;

	/* Send HTTP GET request */
	ret = (cb != NULL)
		? (*cb)(hostname, req, reply)
		: http_request(hostname, req, reply);
	if (ret != MURL_ERR_SUCCESS) {
		free(reply);
		return ret;
	}

	/* Parse server reply */
	res->raw_reply = reply;
	ret = parse_reply(reply, res);

	return ret;
}

#define HTTP_REQ		"GET %s HTTP/1.0\nHost: %s\nUser-Agent: %s/%s\n\n"
#define USER_AGENT		"murl.c"
#define VERSION			"0.1"

static int http_connect(const char *hostname, int *sd);
static int http_send(int sd, char *buf);
static int http_recv(int sd, char *buf);

/*
 * Send HTTP GET request
 */
static int http_request(const char *hostname, const char *req, char *reply)
{
	int sd, ret;
	char *buf;

	/* Connect to the server */
	if ((ret = http_connect(hostname, &sd)) != MURL_ERR_SUCCESS)
		return ret;

	/* Send data */
	if ((buf = murl_sprintf(HTTP_REQ, req, hostname, USER_AGENT,
						VERSION)) != NULL) {
		ret = http_send(sd, buf);
		free(buf);
	} else
		ret = -MURL_ERR_MEM;

	/* Receive data */
	if (ret == MURL_ERR_SUCCESS)
		ret = http_recv(sd, reply);

#ifdef _MSC_VER
	closesocket(sd);
#else
	close(sd);
#endif
	return ret;
}

#ifdef WIN32
/*
 * Win32 helper for the http_connect
 */
void clean_tcp()
{
	while (WSACleanup() == 0);
}
#endif

/*
 * Establish connection to the server
 */
static int http_connect(const char *hostname, int *sd)
{
	struct addrinfo hints;
	struct addrinfo *res, *addr;
	int err, ret = -MURL_ERR_NETWORK;

#ifdef _WIN32
	WSADATA wsaData;

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	atexit(clean_tcp);
#endif

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	
	/* Get server IP */
	err = getaddrinfo(hostname, "http", &hints, &res);
	if (err != 0) {
#ifdef DEBUG
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
#endif
		return ret;
	}

	/* Try to connect to the server */
	for (addr = res; addr != NULL; addr = addr->ai_next) {
		*sd = socket(addr->ai_family, addr->ai_socktype,
			addr->ai_protocol);
		if (*sd == -1) {
#ifdef DEBUG
			perror("socket");
#endif
			continue;
		}
		if (connect(*sd, addr->ai_addr, addr->ai_addrlen) != -1) {
			ret = MURL_ERR_SUCCESS;
			break;
		}
#ifdef DEBUG
		else
			perror("connect");
#endif

#ifdef _MSC_VER
		closesocket(*sd);
#else
		close(*sd);
#endif
	}

#ifdef DEBUG
	if (addr == NULL) {
		fprintf(stderr, "Could not connect\n");
	}
#endif

	freeaddrinfo(res);
	return ret;
}

/*
 * Send data to the server
 */
static int http_send(int sd, char *buf)
{
	int len = strlen(buf);
	int ret = -MURL_ERR_NETWORK;

	if (send(sd, buf, len, 0) == len)
		ret = MURL_ERR_SUCCESS;
#ifdef DEBUG
	else
		perror("send");
#endif

	return ret;
}

/*
 * Get server reply
 */
static int http_recv(int sd, char *buf)
{
	int ret = -MURL_ERR_NETWORK;
	ssize_t len;

	if ((len = recv(sd, buf, BUFSIZE - 1, 0)) < 0) {
		/* An error occured while receiving data */
#ifdef DEBUG
		perror("recv");
#endif
	} else {
		buf[len] = '\0';
		ret = MURL_ERR_SUCCESS;
	}

	return ret;
}

/*
 * Extract field value from the Murl reply
 */
static char *getvalue(char *buf, const char *field)
{
	char *str, *s, *c;
	char *ret = NULL;
	int len = 0;

	if ((str = strdup(buf)) == NULL)
		return ret;

	/* FIXME: check for a duplicate fields */
	if ((s = strstr(str, field)) != NULL &&
		(c = strchr(s, '\n')) != NULL) {
		*c = '\0';
		if ((s = strstr(s, " ")) != NULL && *s++ != '\0')
			len = strlen(s);
		if (len > 0 && (ret = malloc(len + 1)) != NULL)
			if (strcpy(ret, s) == NULL)
				free(ret);
	}

	free(str);
	return ret;
}

/*
 * Parse server reply
 */
static int parse_reply(char *reply, struct murl_response *res)
{
	char *v, *msg, *url = NULL;
	int status = MURL_UNKNOWN;
	int code = -1;
	int ret = -MURL_ERR_PARSE;

	/* result */
	if ((v = getvalue(reply, "result")) == NULL)
		return ret;

	if (strcmp(v, "OK") == 0)
		status = MURL_OK;
	else if (strcmp(v, "ERROR") == 0)
		status = MURL_ERROR;
	free(v);

	/* code */
	if ((v = getvalue(reply, "code")) == NULL)
		return ret;

	code = atoi(v);
	free(v);
	if (code == -1)
		return ret; /* Conversion error */

	/* message */
	if ((msg = getvalue(reply, "message")) == NULL)
		return ret;

	/* url */
	if (status == MURL_OK)
		if ((url = getvalue(reply, "url")) == NULL) {
			free(msg);
			return ret;
		}

	res->status = status;
	res->code = code;
	res->message = msg;
	res->url = url;
	ret = MURL_ERR_SUCCESS;

	return ret;
}

/*
 * sprintf like function to help build API strings
 *
 * FIXME: parses '%s' conversion specifier only
 */
static char *murl_sprintf(const char *fmt, ...)
{
	va_list ap;
	char *pfmt = (char *) fmt, *ret = NULL;
	int len = 0, i = 0;

	va_start(ap, fmt);
	while (*pfmt) {
		if (*pfmt == '%' && *(++pfmt) == 's') {
			len += strlen(va_arg(ap, char *));
			i++;
		}
		pfmt++;
	}
	va_end(ap);

	if (len > 0) {
		len += strlen(fmt) - (i * 2);
		if ((ret = malloc(len + 1)) != NULL) {
			va_start(ap, fmt);
			if (vsprintf(ret, fmt, ap) != len)
				free(ret);
			va_end(ap);
		}
	}

	return ret;
}
