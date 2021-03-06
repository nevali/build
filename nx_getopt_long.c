/* Copyright 2010 Mo McRoberts.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nx_getopt_long.h"

const char *nx_longopt = NULL;

int
nx_getopt_long(int argc, char *const *argv, const char *shortopts, const struct nx_option *longopts, int *indexptr)
{
	int lopterr, r, c;
	char *op, *ap;
	static char buf[32];
	size_t l;
	static int lastlong = -1;

	lopterr = opterr;
	opterr = optopt = 0;
	nx_longopt = NULL;
	if(indexptr)
	{
		*indexptr = -1;
	}
	do
	{
		c = optind >= 1 ? optind : 1;
		r = getopt(argc, argv, shortopts);
		if(r == EOF)
		{
			return r;
		}
	}
	while(c == lastlong);
	if(r != '?')
	{
		opterr = lopterr;
		return r;
	}
	if(argv[c][1] == '-')
	{
		if(argv[c][2] == 0)
		{
			/* Rest argument */
			return EOF;
		}
		lastlong = c;
		op = argv[c] + 2;
		if((ap = strchr(op, '=')))
		{
			l = ap - op;
		}
		else
		{
			l = strlen(op);
		}
		strncpy(buf, op, l >= sizeof(buf) ? sizeof(buf) - 1 : l);
		buf[(l >= sizeof(buf) ? sizeof(buf) - 1 : l)] = 0;
		optarg = (ap ? ap + 1 : NULL);
		if(longopts)
		{
			for(c = 0; longopts[c].name; c++)
			{				
				if(strlen(longopts[c].name) != l || strncmp(op, longopts[c].name, l))
				{
					continue;
				}
				nx_longopt = longopts[c].name;
				if(indexptr)
				{
					*indexptr = c;
				}
				optopt = longopts[c].val;
				if(longopts[c].flag)
				{
					*(longopts[c].flag) = optopt ? optopt : 1;
				}
				switch(longopts[c].has_arg)
				{
				case no_argument:
					if(ap)
					{
						if(lopterr)
						{				 
							fprintf(stderr, "%s: option `--%s` doesn't allow an argument\n", argv[0], longopts[c].name);
						}
						opterr = lopterr;
						return '?';
					}
					break;
				case optional_argument:
					break;
				case required_argument:
					if(!ap)
					{
						if(lopterr)
						{
							fprintf(stderr, "%s: option `--%s` requires an argument\n", argv[0], longopts[c].name);
						}
						opterr = lopterr;
						return '?';
					}
				}
				return optopt;						
			}
		}
		fprintf(stderr, "buf=[%s]\n", buf);
		nx_longopt = buf;
		if(lopterr)
		{
			fprintf(stderr, "%s: unrecognized option `--%s`\n", argv[0], buf);
		}
		opterr = lopterr;
		return '?';
	}
	if(lopterr)
	{
		fprintf(stderr, "%s: unrecognized option `-%c'\n", argv[0], optopt);
	}
	opterr = lopterr;
	return '?';
}
