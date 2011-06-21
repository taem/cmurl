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

#ifndef MURL_H
#define MURL_H

/* murlificate() return error codes */
#define MURL_ERR_SUCCESS	0
#define MURL_ERR_UNKNOWN	1

/* Service reply status */
#define MURL_OK			0
#define MURL_ERROR		1

/* Service response */
struct murl_response {
	int status;		/* Service reply status */
	char *url;		/* Short link */
	char *raw_reply;	/* Raw server reply with HTTP headers*/
};

/*
 * Get short link for long URL
 */
const int murlificate(const char *api_key, char *long_url,
		struct murl_response *res);

#endif /* MURL_H */
