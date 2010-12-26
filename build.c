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

#include "p_build.h"

static const char *progname;

static struct option longopts[] = {
	{ "dir", required_argument, NULL, 'C' },
	{ "project", required_argument, NULL, 'p' },
	{ "product", required_argument, NULL, 'P' },
	{ "build", required_argument, NULL, 'B' },
	{ "host", required_argument, NULL, 'H' },
	{ "target", required_argument, NULL, 'T' },
	{ "config", required_argument, NULL, 'c' },
	{ "sdk", required_argument, NULL, 's' },
	{ "only", no_argument, NULL, 'O' },
	{ "dry-run", no_argument, NULL, 'N' },
	{ "at", required_argument, NULL, 'r' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "quiet", no_argument, NULL, 'q' },
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'V' },
	{ NULL, 0, NULL, 0 }
};

static void
usage(void)
{
	fprintf(stderr, "Usage:\n"
			"\n"
			"%s\n"
			"      [-CDIR|--dir=DIR]             Change to DIR before commencing\n"
			"      [-pPATH|--project=PATH]       Specify path to a project file or directory\n"
			"      [-PNAME|--product=NAME]       Build the named product NAME\n"
			"      [-BTRIPLET|--build=TRIPLET]   Set the build system type to TRIPLET\n"
			"      [-HTRIPLET|--host=TRIPLET]    Set the host system type to TRIPLET\n"
			"      [-TTRIPLET|--target=TRIPLET]  Set the target system type to TRIPLET\n"
			"      [-cNAME|--config=NAME]        Build using the configuration named NAME\n"
			"      [-sPATH|--sdk=PATH]           Build using the SDK found at PATH\n"
			"      [-DVAR[=VALUE]]               Define the variable VAR (optionally to VALUE)\n"
			"      [-O|--only]                   Do not attempt prerequisite build phases\n"
			"      [-N|--dry-run]                Don't actually execute anything\n"
			"      [-r[USER@]HOST|--at=[USER@]HOST]\n"
			"                                    Invoke %s on a remote host\n"
			"      [-v|--verbose]                Print information about actions\n"
			"      [-q|--quiet]                  Be as quiet as possible\n"
			"      [PHASE]                       Specify the build phase PHASE\n"
			"\n"
			"%s --help|-h                     Show this usage information\n"
			"\n"
			"%s --version|-V                  Show version information\n"
			"\n"
			"PHASE is one of:\n"
			"      prepare                       Prepare the project for building\n"
			"                                    (e.g., run autoconf, automake, etc.)\n"
			"      config                        Configure the project for current host\n"
			"      build                         Actually build the project\n"
			"      install                       Install the built project\n"
			"      clean                         Remove 'build' output\n"
			"      distclean                     Remove 'build' and 'config' output\n",
			progname, progname, progname, progname);
}

static void
version(void)
{
	printf("%s -- development version\n", progname);
}

void
parse_options(int argc, char **argv, build_context_t *context)
{
	int r;

	while((r = getopt_long(argc, argv, "hVONrvqD:C:P:B:H:T:c:s:", longopts, NULL)) != EOF)
	{
		switch(r)
		{
		case 'h':
			usage();
			exit(0);
		case 'V':
			version();
			exit(0);
		case 'C':
			context->wd = optarg;
			break;
		case 'p':
			context->project = optarg;
			break;
		case 'P':
			context->product = optarg;
			break;
		case 'B':
			context->build = optarg;
			break;			
		case 'H':
			context->host = optarg;
			break;
		case 'T':
			context->target = optarg;
			break;
		case 'c':
			context->config = optarg;
			break;
		case 's':
			context->sdk = optarg;
			break;
		case 'O':
			context->only = 1;
			break;
		case 'r':
			context->remote = optarg;
			break;
		case 'N':
			context->dryrun = 1;
			context->verbose = 1;
			context->quiet = 0;
			break;
		case 'v':
			context->verbose = 1;
			context->quiet = 0;
			break;
		case 'q':
			context->verbose = 0;
			context->quiet = 1;
			break;
		case '?':
			exit(EXIT_FAILURE);
		default:
			fprintf(stderr, "r=%c, optind=%d, optarg=%s\n", r, optind, optarg);
		}
	}
}

int
main(int argc, char **argv)
{
	char *t;
	build_context_t context;
	int r, here;

	if((t = strrchr(argv[0], '/')))
	{
		progname = t + 1;
	}
	else
	{
		progname = t;
	}
	memset(&context, 0, sizeof(build_context_t));
	context.progname = progname;
	parse_options(argc, argv, &context);
	if((here = context_chdir(&context)) < 0)
	{
		exit(EXIT_FAILURE);
	}
	if(context.project)
	{
		if(stat(context.project, &context.sbuf) < 0)
		{
			fprintf(stderr, "%s: %s: %s\n", progname, context.project, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	if(NULL == context_detect(&context))
	{
		fprintf(stderr, "%s: *** No suitable project file or directory could be found.  Stop.\n", progname);
		exit(EXIT_FAILURE);
	}
	if(optind >= argc)
	{
		return context.vt->build(&context);
	}
	for(; optind < argc; optind++)
	{
		if(!strcmp(argv[optind], "prepare"))
		{
			if((r = context.vt->prepare(&context)))
			{
				return r;
			}
		}
		else if(!strcmp(argv[optind], "config"))
		{
			if((r = context.vt->config(&context)))
			{
				return r;
			}
		}
		else if(!strcmp(argv[optind], "build"))
		{
			if((r = context.vt->build(&context)))
			{
				return r;
			}
		}
		else if(!strcmp(argv[optind], "install"))
		{
			if((r = context.vt->install(&context)))
			{
				return r;
			}
		}
		else if(!strcmp(argv[optind], "clean"))
		{
			if((r = context.vt->clean(&context)))
			{
				return r;
			}
		}
		else if(!strcmp(argv[optind], "distclean"))
		{
			if((r = context.vt->distclean(&context)))
			{
				return r;
			}
		}
		else
		{
			fprintf(stderr, "%s: unrecognized build phase `%s'\n", progname, argv[optind]);
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}
