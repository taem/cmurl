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

#include "murl.h"


int main(int argc, char **argv)
{
	struct murl_response res;
	int err;

	if (argc < 3) {
		fprintf(stderr, "%s: Too few arguments.\n", argv[0]);
		fprintf(stderr, "Usage: cmurl <api_key> <long_url>\n");
		exit(EXIT_FAILURE);
	}

	err = murlificate(argv[1], argv[2], &res);
	if (err != MURL_ERR_SUCCESS) {
		fprintf(stderr, "ERROR: something went wrong :(\n");
		exit(EXIT_FAILURE);
	}

	switch (res.status) {
	case MURL_OK:
		printf("%s\n", res.url);
		free(res.url);
		break;
	case MURL_ERROR:
		printf("ERROR: cannot get short link\n");
		break;
	}

	free(res.raw_reply);
	exit(EXIT_SUCCESS);
}
