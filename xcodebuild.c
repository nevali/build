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
	build_defn_t *p;

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
	for(p = ctx->defs; p; p = p->hh.next)
	{
		if(p->value)
		{
			cmd_arg_addf(cmd, "%s=%s", p->name, p->value);
		}
		else
		{
			cmd_arg_addf(cmd, "%s=1", p->name);
		}
	}
}

int
xcodebuild_build(build_context_t *ctx)
{
	cmd_t *cmd;
	int r;

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
	int r;

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

build_handler_t xcodebuild_handler = {
	"xcodebuild",
	"Builds Apple Xcode projects via xcodebuild",
	xcodebuild_detect,
	NULL,
	NULL,
	xcodebuild_build,
	xcodebuild_install,
	xcodebuild_clean,
	NULL,
};


#endif /*ENABLE_XCODEBUILD*/
