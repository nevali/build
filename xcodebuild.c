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

#ifdef ENABLE_XCODEBUILD

int
xcodebuild_detect(build_context_t *ctx)
{
	static char *pbuf;
	size_t l;
	char *match, *p;
	DIR *d;
	struct dirent *de;

	if(ctx->project && !S_ISDIR(ctx->sbuf.st_mode))
	{
		/* We could theoretically support specifying the path to
		 * projectname.xcodeproj/project.pbxproj, but not right now.
		 */
		return 0;
	}
	if(ctx->project)
	{
		l = strlen(ctx->project);
		if(pbuf)
		{
			free(pbuf);
		}
		if(!(pbuf = (char *) calloc(1, l + 32)))
		{
			return -1;
		}
		strcpy(pbuf, ctx->project);
		strcpy(&(pbuf[l]), "/project.pbxproj");
		if(!access(pbuf, R_OK))
		{			
			/* ctx->project points to a .xcodeproj */
			fprintf(stderr, "%s found; %s is a valid project\n", pbuf, ctx->project);
			return 1;
		}
		chdir(ctx->project);
	}
	match = NULL;
	d = opendir(".");
	/* Scan the directory for .xcodeproj files. We need to find exactly
	 * one.
	 */
	while((de = readdir(d)))
	{
		if(strlen(de->d_name) > 10 && (p = strrchr(de->d_name, '.')))
		{
			if(!strcmp(p, ".xcodeproj"))
			{
				if(match)
				{
					fprintf(stderr, "%s: *** Multiple Xcode project file matches found -- specify one with `--project=NAME'\n", ctx->progname);
					return -1;
				}
				if(pbuf)
				{
					free(pbuf);
				}
				match = pbuf = strdup(de->d_name);
			}
		}		
	}
	closedir(d);
	if(match)
	{
		ctx->project = match;
		return 1;
	}
	context_returnwd(ctx);
	return 0;
}

static void
xcodebuild_args(cmd_t *cmd, build_context_t *ctx, const char *phase)
{
	if(ctx->project)
	{
		cmd_arg_add(cmd, "-project");
		cmd_arg_add(cmd, ctx->project);
	}
	if(ctx->product)
	{
		cmd_arg_add(cmd, "-target");
		cmd_arg_add(cmd, ctx->product);
	}
	if(ctx->config)
	{
		cmd_arg_add(cmd, "-configuration");
		cmd_arg_add(cmd, ctx->config);
	}
	if(ctx->sdk)
	{
		cmd_arg_add(cmd, "-sdk");
		cmd_arg_add(cmd, ctx->sdk);
	}
	if(phase)
	{
		cmd_arg_add(cmd, phase);
	}
}

int
xcodebuild_prepare(build_context_t *ctx)
{
	if(ctx->prepared)
	{
		return 0;
	}
	ctx->prepared = 1;
	if(!ctx->quiet && (!ctx->isauto || ctx->verbose))
	{
		fprintf(stderr, "%s: Nothing to be done for phase 'prepare'\n", ctx->progname);
	}
	return 0;
}

int
xcodebuild_config(build_context_t *ctx)
{	
	int r, a;

	if(ctx->configured)
	{
		return 0;
	}
	AUTODEP(ctx, a, r, r = ctx->vt->prepare(ctx));
	ctx->prepared = 1;
	if(!ctx->quiet && (!ctx->isauto || ctx->verbose))
	{
		fprintf(stderr, "%s: Nothing to be done for phase 'config'\n", ctx->progname);
	}
	return 0;
}


int
xcodebuild_build(build_context_t *ctx)
{
	cmd_t *cmd;
	int r, a;

	if(ctx->built)
	{
		return 0;
	}
	AUTODEP(ctx, a, r, r = ctx->vt->config(ctx));
	ctx->built = 1;
	cmd = context_cmd_create(ctx, "xcodebuild", NULL, "BUILD_XCODEBUILD", "XCODEBUILD", NULL);
	xcodebuild_args(cmd, ctx, "build");
	r = cmd_spawn(cmd, 0);
	cmd_destroy(cmd);
	return r;
}

int
xcodebuild_install(build_context_t *ctx)
{
	cmd_t *cmd;
	int r, a;

	if(ctx->installed)
	{
		return 0;
	}
	AUTODEP(ctx, a, r, r = ctx->vt->build(ctx));
	ctx->installed = 1;
	cmd = context_cmd_create(ctx, "xcodebuild", NULL, "BUILD_XCODEBUILD", "XCODEBUILD", NULL);
	xcodebuild_args(cmd, ctx, "install");
	r = cmd_spawn(cmd, 0);
	cmd_destroy(cmd);
	return r;
}

int
xcodebuild_clean(build_context_t *ctx)
{
	cmd_t *cmd;
	int r;

	ctx->built = 0;
	ctx->installed = 0;
	cmd = context_cmd_create(ctx, "xcodebuild", NULL, "BUILD_XCODEBUILD", "XCODEBUILD", NULL);
	xcodebuild_args(cmd, ctx, "clean");
	r = cmd_spawn(cmd, ctx->isauto);
	cmd_destroy(cmd);
	return r;
}

int
xcodebuild_distclean(build_context_t *ctx)
{
	if(!ctx->quiet && (!ctx->isauto || ctx->verbose))
	{
		fprintf(stderr, "%s: Nothing to be done for phase 'distclean'\n", ctx->progname);
	}
	return 0;
}



build_handler_t xcodebuild_handler = {
	xcodebuild_detect,
	xcodebuild_prepare,
	xcodebuild_config,
	xcodebuild_build,
	xcodebuild_install,
	xcodebuild_clean,
	xcodebuild_distclean,
};


#endif /*ENABLE_XCODEBUILD*/
