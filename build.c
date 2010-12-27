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

int makelevel;

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
			"      [VAR=VALUE] ...               Define the variable VAR to VALUE\n"
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
	fprintf(stderr, "\nAvailable handlers:\n\n");
	context_handler_list(stderr);
	fprintf(stderr, "\n");
}

static void
version(void)
{
	printf("%s -- development version\n", progname);
}

void
parse_options(int argc, char **argv, build_context_t *context)
{
	/* These are variable names which may be used as long options.
	 * i.e., rather than specifying -Dprefix=/opt/local, you can
	 * specify --prefix=/opt/local.
	 */
	static const char *aliases[] = {
		"prefix", "exec-prefix", "bindir", "sbindir", "libexecdir",
		"sysconfdir", "sharedstatedir", "localstatedir", "libdir",
		"includedir", "oldincludedir", "datarootdir", "datadir",
		"infodir", "localedir", "mandir", "docdir", "htmldir",
		"dvidir", "pdfdir", "psdir", "program-prefix",
		"program-suffix", "program-transform-name",
		NULL
	};
	int r, c, match, idx;
	char *p;
	
	opterr = 0;
	while((r = getopt_long(argc, argv, "hVONrvqD:C:P:B:H:T:c:s:", longopts, &idx)) != EOF)
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
		case 'D':
			if((p = strchr(optarg, '=')))
			{
				*p = 0;
				p++;
			}
			context_defn_add(context, optarg, p);
			break;
		case '?':
			if(longopt)
			{
				if(idx != -1)
				{
					if(longopts[idx].has_arg == no_argument)
					{
						fprintf(stderr, "%s: option `--%s' doesn't allow an argument\n", context->progname, longopt);
					}
					else if(longopts[idx].has_arg == required_argument)
					{
						fprintf(stderr, "%s: option `--%s' requires an argument\n", context->progname, longopt);
					}
					else
					{
						fprintf(stderr, "%s: an unknown error occurred processing the option `--%s'\n", context->progname, longopt);
					}
					exit(EXIT_FAILURE);
				}
				if(!strncmp(longopt, "with-", 5) || !strncmp(longopt, "without-", 8) ||
				   !strncmp(longopt, "enable-", 7) || !strncmp(longopt, "disable-", 7))
				{
					context_defn_add(context, longopt, optarg);
					continue;
				}
				match = 0;
				for(c = 0; aliases[c]; c++)
				{
					fprintf(stderr, "comparing %s and %s\n", longopt, aliases[c]);
					if(!strcmp(longopt, aliases[c]))
					{
						context_defn_add(context, aliases[c], optarg);
						match = 1;
						break;
					}
				}
				if(match)
				{
					continue;
				}
				fprintf(stderr, "%s: unrecongnized option `--%s`\n", context->progname, longopt);
				exit(EXIT_FAILURE);
			}
			fprintf(stderr, "%s: unrecognized option `-%c'\n", context->progname, optopt);
			exit(EXIT_FAILURE);
		default:
			fprintf(stderr, "[unhandled option: r=%c, optind=%d, optarg=%s]\n", r, optind, optarg);
		}
	}
	for(c = optind; c < argc; c++)
	{
		if(!strcmp(argv[c], "-" ) || !strcmp(argv[c], "--"))
		{
			continue;
		}
		if((p = strchr(argv[c], '=')))
		{
			*p = 0;
			p++;
			context_defn_add(context, argv[c], p);
			*p = '=';
		}
	}		
}

int
main(int argc, char **argv)
{
	char *t, buf[64];
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
	if((t = getenv("MAKELEVEL")))
	{
		context.level = atoi(t);
	}
	sprintf(buf, "%d", context.level + 1);
	setenv("MAKELEVEL", buf, 1);
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
		context_msg(&context, MSG_FATAL, "No suitable project file or directory could be found.  Stop.\n");
		exit(EXIT_FAILURE);
	}
	if(optind >= argc)
	{
		return context_build(&context, PH_BUILD, 0);
	}
	for(; optind < argc; optind++)
	{
		if(!strcmp(argv[optind], "-" ) || !strcmp(argv[optind], "--") || strchr(argv[optind], '='))
		{
			continue;
		}
		if(!strcmp(argv[optind], "prepare"))
		{
			r = context_build(&context, PH_PREPARE, 0);
		}
		else if(!strcmp(argv[optind], "config"))
		{
			r = context_build(&context, PH_CONFIG, 0);
		}
		else if(!strcmp(argv[optind], "build"))
		{
			r = context_build(&context, PH_BUILD, 0);
		}
		else if(!strcmp(argv[optind], "install"))
		{
			r = context_build(&context, PH_INSTALL, 0);
		}
		else if(!strcmp(argv[optind], "clean"))
		{
			r = context_build(&context, PH_CLEAN, 0);
		}
		else if(!strcmp(argv[optind], "distclean"))
		{
			r = context_build(&context, PH_DISTCLEAN, 0);
		}
		else
		{
			context_msg(&context, MSG_FATAL, "unrecognized build phase `%s'\n", argv[optind]);
			exit(EXIT_FAILURE);
		}
		if(r)
		{
			return (r < 0 ? 1 : r);
		}
	}
	return 0;
}
