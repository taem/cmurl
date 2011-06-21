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

#ifndef MURL_H
#define MURL_H

/* murlificate() return error codes */
#define MURL_ERR_SUCCESS	0
#define MURL_ERR_MEM		1
#define MURL_ERR_NETWORK	2
#define MURL_ERR_PARSE		3

/* API response status */
#define MURL_OK			0
#define MURL_ERROR		1
#define MURL_UNKNOWN		2

/* API response */
struct murl_response {
	int status;		/* Response status */
	int code;		/* Status code: 200, 400, etc */
	char *url;		/* Short link */
	char *message;		/* Human readable message */
	char *raw_reply;	/* Server reply with HTTP headers */
};

/* HTTP request callback */
typedef int (*murl_http_request) (const char *hostname, const char *req,
				char *reply);

/*
 * Get short link for long URL
 */
const int murlificate(const char *api_key, const char *url,
		struct murl_response *res, murl_http_request cb);

#endif /* MURL_H */
