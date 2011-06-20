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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
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

#define	MURL_API_HOST		"api.murl.kz"
#define	MURL_API_HANDLER	"/basic"
#define	MURL_API_FORMAT		"text"
#define MURL_API_STRING		"%s?format=%s&api_key=%s&url=%s"


static char *build_url(const char *api_key, char *long_url);
static char *http_request(const char *hostname, const char *query);
static int parse_reply(char *reply, struct murl_response *res);

/*
 * Get short link for long URL
 */
const int murlificate(const char *api_key, char *long_url,
		struct murl_response *res)
{
	char *api_url, *reply;

	/* Build URL */
	if ((api_url = build_url(api_key, long_url)) == NULL)
		return -MURL_ERR_UNKNOWN;

	/* Send HTTP GET request */
	reply = http_request(MURL_API_HOST, api_url);
	free(api_url);
	if (reply == NULL)
		return -MURL_ERR_UNKNOWN;

	/* Parse server reply */
	if (parse_reply(reply, res) == -1) {
		free(reply);
		return -MURL_ERR_UNKNOWN;
	}

	return MURL_ERR_SUCCESS;
}

extern char *url_encode(char *str);

/*
 * Build API URL
 */
static char *build_url(const char *api_key, char *long_url)
{
	char *long_url_encoded, *ret;
	int len;

	long_url_encoded = url_encode(long_url);
	len = (strlen(MURL_API_STRING) - 8) +
		strlen(MURL_API_HANDLER) +
		strlen(MURL_API_FORMAT) +
		strlen(api_key) +
		strlen(long_url_encoded);

	if ((ret = malloc(len + 1)) != NULL)
		if (sprintf(ret, MURL_API_STRING, MURL_API_HANDLER,
				MURL_API_FORMAT, api_key, long_url_encoded) < 0)
			free(ret);

	free(long_url_encoded);
	return ret;
}

static int http_connect(const char *hostname);
static int http_send(const char *str, int sd);
static char *http_recv(int sd);

/*
 * Send HTTP GET request
 */
static char *http_request(const char *hostname, const char *query)
{
	int sd;
	char *ret = NULL;

	/* Connect to the server */
	if ((sd = http_connect(hostname)) == -1)
		return ret;

	/* Send data */
	if (http_send(query, sd) != -1)
		/* Receive data */
		ret = http_recv(sd);

	close(sd);
	return ret;
}

/*
 * Establish a connection to the server
 */
static int http_connect(const char *hostname)
{
	struct addrinfo hints;
	struct addrinfo *res, *addr;
	int err, sd, ret = -1;

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
		sd = socket(addr->ai_family, addr->ai_socktype,
			addr->ai_protocol);
		if (sd == -1) {
#ifdef DEBUG
			perror("socket");
#endif
			continue;
		}
		if (connect(sd, addr->ai_addr, addr->ai_addrlen) != -1) {
			ret = sd;
			break;
		}
#ifdef DEBUG
		else
			perror("connect");
#endif
		close(sd);
	}

#ifdef DEBUG
	if (addr == NULL) {
		fprintf(stderr, "Could not connect\n");
	}
#endif

	freeaddrinfo(res);
	return ret;
}

#define HTTP_GET_REQ		"GET %s HTTP/1.0\nHost: %s\nUser-Agent: %s\n\n"
#define USER_AGENT		"murl.c"

/*
 * Send request to the server
 */
static int http_send(const char *str, int sd)
{
	int len, ret = -1;
	char *get;

	len = (strlen(HTTP_GET_REQ) - 6) +
		strlen(str) +
		strlen(MURL_API_HOST) +
		strlen(USER_AGENT);

	if ((get = malloc(len + 1)) != NULL) {
		if (sprintf(get, HTTP_GET_REQ, str, MURL_API_HOST,
				USER_AGENT) > 0)
			if (send(sd, get, len, 0) == len)
				ret = 0;
#ifdef DEBUG
			else
				perror("send");
#endif
		free(get);
	}

	return ret;
}

#define BUF_SIZE		2048

/*
 * Get server reply
 */
static char *http_recv(int sd)
{
	char *ret;
	ssize_t nrecv;

	if ((ret = malloc(BUF_SIZE)) == NULL)
		return ret;

	if ((nrecv = recv(sd, ret, BUF_SIZE - 1, 0)) < 0) {
		/* An error occured while receiving data */
		free(ret);
#ifdef DEBUG
		perror("recv");
#endif
	} else
		ret[nrecv] = '\0';

	return ret;
}

/*
 * Parse server reply
 */
static int parse_reply(char *reply, struct murl_response *res)
{
	char *c, *path = "http://murl.kz/";
	int len = strlen(path) + 4;
	int ret = -1;

	if ((res->url = malloc(len + 1)) == NULL)
		return ret;

	ret = 0;
	res->status = MURL_ERROR;
	res->raw_reply = reply;

	if ((c = strstr(reply, path)) != NULL &&
		strncpy(res->url, c, len) != NULL) {
		res->url[len] = '\0';
		res->status = MURL_OK;
	} else
		free(res->url);

	return ret;
}
