// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
//           2007 Luca Longinotti <chtekk@gentoo.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <unistd.h>
#include <dirent.h>
#include <confuse.h>

#include "auth.h"
#include "cfg.h"
#include "methods.h"

#include <lucid/mem.h>
#include <lucid/misc.h>
#include <lucid/log.h>
#include <lucid/open.h>
#include <lucid/str.h>

static cfg_opt_t MOUNT_OPTS[] = {
	CFG_STR("src", NULL, CFGF_NONE),
	CFG_STR("type", NULL, CFGF_NONE),
	CFG_STR("opts", NULL, CFGF_NONE),
	CFG_END()
};

static cfg_opt_t BUILD_OPTS[] = {
	CFG_STR("description", NULL, CFGF_NONE),

	CFG_STR("init",    NULL, CFGF_NONE),
	CFG_STR("halt",    NULL, CFGF_NONE),
	CFG_STR("reboot",  NULL, CFGF_NONE),
	CFG_INT("timeout", 0,    CFGF_NONE),

	CFG_SEC("mount", MOUNT_OPTS, CFGF_MULTI|CFGF_TITLE|CFGF_NONE),

	CFG_STR_LIST("vx_bcaps", NULL, CFGF_NONE),
	CFG_STR_LIST("vx_ccaps", NULL, CFGF_NONE),
	CFG_STR_LIST("vx_flags", NULL, CFGF_NONE),
	CFG_END()
};

static
const char *template_description(const char *tbasedir, const char *name)
{
	char *tconf = NULL;
	asprintf(&tconf, "%s/%s.conf", tbasedir, name);

	if (str_isempty(tconf) || !isfile(tconf))
		return "(none)";

	cfg_t *tcfg = cfg_init(BUILD_OPTS, CFGF_NOCASE);

	switch (cfg_parse(tcfg, tconf)) {
	case CFG_FILE_ERROR:
	case CFG_PARSE_ERROR:
		mem_free(tconf);
		return "(parse error in configuration)";

	default:
		mem_free(tconf);
		break;
	}

	const char *description = cfg_getstr(tcfg, "description");

	return str_isempty(description) ? "(none)" : description;
}

/* vx.templates([string name]) */
xmlrpc_value *m_vx_templates(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name;

	params = method_init(env, p, c, VCD_CAP_CREATE, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,*}",
			"name", &name);
	method_return_if_fault(env);

	int curfd = open_read(".");

	/* get template dir */
	const char *tbasedir = cfg_getstr(cfg, "templatedir");

	if (chdir(tbasedir) == -1)
		method_return_sys_faultf(env, "chdir(%s)", tbasedir);

	DIR *tfd = opendir(".");

	if (tfd == NULL)
		method_return_sys_faultf(env, "opendir(%s)", tbasedir);

	struct dirent *dent;
	xmlrpc_value *response = xmlrpc_array_new(env);

	/* list all templates */
	while ((dent = readdir(tfd))) {
		if (str_equal(dent->d_name, ".") || str_equal(dent->d_name, "..") ||
				(!str_isempty(name) && !str_equal(name, dent->d_name)))
			continue;

		if (isdir(dent->d_name)) {
			const char *description = template_description(tbasedir,
					dent->d_name);

			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
					"{s:s,s:s}",
					"name",        dent->d_name,
					"description", description));
		}
	}

	closedir(tfd);
	fchdir(curfd);
	close(curfd);

	return response;
}
