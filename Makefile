#
# Murl.kz API implementation
#
# Copyright (C) 2011 Timur Birsh <taem@linukz.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Porting credits:
#   Windows: resurtm <resurtm@gmail.com>

UNAME := $(shell uname -s)

cmurl: cmurl.o murl.o urlcode.o
ifeq ($(UNAME), MINGW32_NT-6.1)
	gcc -o cmurl cmurl.o murl.o urlcode.o -lws2_32 -lwsock32
else
	gcc -o cmurl cmurl.o murl.o urlcode.o
endif

cmurl.o: cmurl.c
	gcc -g -c cmurl.c

murl.o: murl.c
	gcc -DDEBUG -g -c murl.c

urlcode.o: urlcode.c
	gcc -g -c urlcode.c

clean:
	rm -f *.o cmurl
