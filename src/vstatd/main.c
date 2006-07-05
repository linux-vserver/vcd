#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "log.h"
#include "cfg.h"
#include "lucid.h"

int checkdir (char *dirname);

static cfg_opt_t CFG_OPTS[] = {   
    CFG_STR("statsdir", "/proc/virtual", CFGF_NONE),
    CFG_STR("logfile", "/var/log/vstatd.log", CFGF_NONE),
    CFG_STR("dbdir", "/etc/vstatd/", CFGF_NONE),   
    CFG_END()
};

cfg_t *cfg;

int main (int argc, char *argv[]) {
    char *cfg_file = "/etc/vstatd.conf";
   
    cfg = cfg_init(CFG_OPTS, CFGF_NOCASE);

    int debug = 0;
    char *statsdir;
   
    switch (cfg_parse(cfg, cfg_file)) {
     case CFG_FILE_ERROR:
      perror("cfg_parse");
      exit(EXIT_FAILURE);
     case CFG_PARSE_ERROR:
      fprintf(stderr, "cfg_parse: Parse error\n");
      exit(EXIT_FAILURE);
     default:
      break;
    }
   
    if (log_init(debug) < 0) {
       perror("log_init");
       exit(EXIT_FAILURE);
    }
   
    statsdir = cfg_getstr(cfg, "statsdir");
   
    checkdir(statsdir);
}
