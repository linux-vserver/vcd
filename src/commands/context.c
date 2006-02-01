/***************************************************************************
 *   Copyright 2005-2006 by the vserver-utils team                         *
 *   See AUTHORS for details                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* getopt settings */
#define CONTEXT_SHORT_OPTS "a" COMMON_SHORT_OPTS

static struct option const context_long_opts[] = {
	COMMON_LONG_OPTS
};

static const char context_rcsid[] = "$Id$";

static
void context_usage(int rc)
{
	vu_printf("usage...\n");
	exit(rc);
}

int context_main(int argc, char **argv)
{
	int c;
	
	while (1) {
		c = GETOPT_LONG(CONTEXT, context);
		if (c == -1) break;
		
		
		switch (c) {
			COMMON_GETOPT_CASES(context)
		}
	}
	
	return EXIT_SUCCESS;
}
