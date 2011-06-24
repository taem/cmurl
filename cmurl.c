/*
 * cmurl - CLI client for Murl.kz
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "murl.h"


int main(int argc, char **argv)
{
	char *url, *s, *proto = "http://";
	struct murl_response res;
	int err;

	if (argc < 3) {
		fprintf(stderr, "%s: Too few arguments.\n", argv[0]);
		fprintf(stderr, "Usage: cmurl <api_key> <long_url>\n");
		exit(EXIT_FAILURE);
	}

	url = argv[2];
	if (strstr(url, proto) == NULL) {
		if ((s = malloc(strlen(proto) + strlen(url) + 1)) == NULL) {
			fprintf(stderr, "Cannot allocate memory.\n");
			exit(EXIT_FAILURE);
		}
		strcat(s, proto);
		strcat(s, url);
	} else
		s = url;
	err = murlificate(argv[1], s, &res, NULL);
	if (err != MURL_ERR_SUCCESS) {
		fprintf(stderr, "ERROR: ");
		if (err == -MURL_ERR_PARSE) {
			fprintf(stderr, "cannot parse reply:\n%s\n", res.raw_reply);
			free(res.raw_reply);
		} else
			fprintf(stderr, "something went wrong :(\n");
		exit(EXIT_FAILURE);
	}

	switch (res.status) {
	case MURL_OK:
		printf("%s\n", res.url);
		break;
	case MURL_ERROR:
		printf("ERROR: %s\n", res.message);
		break;
	case MURL_UNKNOWN:
		printf("ERROR: unknown response status: %s\n", res.message);
		break;
	}

	free(res.message);
	if (res.status == MURL_OK)
		free(res.url);
	free(res.raw_reply);
	exit(EXIT_SUCCESS);
}
