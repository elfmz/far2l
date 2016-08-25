/*
 * cmdline.c - command-line parsing shared between many of the
 * PuTTY applications
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "putty.h"

/*
 * Some command-line parameters need to be saved up until after
 * we've loaded the saved session which will form the basis of our
 * eventual running configuration. For this we use the macro
 * SAVEABLE, which notices if the `need_save' parameter is set and
 * saves the parameter and value on a list.
 * 
 * We also assign priorities to saved parameters, just to slightly
 * ameliorate silly ordering problems. For example, if you specify
 * a saved session to load, it will be loaded _before_ all your
 * local modifications such as -L are evaluated; and if you specify
 * a protocol and a port, the protocol is set up first so that the
 * port can override its choice of port number.
 * 
 * (In fact -load is not saved at all, since in at least Plink the
 * processing of further command-line options depends on whether or
 * not the loaded session contained a hostname. So it must be
 * executed immediately.)
 */

#define NPRIORITIES 2

struct cmdline_saved_param {
    char *p, *value;
};
struct cmdline_saved_param_set {
    struct cmdline_saved_param *params;
    int nsaved, savesize;
};

/*
 * C guarantees this structure will be initialised to all zero at
 * program start, which is exactly what we want.
 */
static struct cmdline_saved_param_set saves[NPRIORITIES];

static void cmdline_save_param(char *p, char *value, int pri)
{
    if (saves[pri].nsaved >= saves[pri].savesize) {
	saves[pri].savesize = saves[pri].nsaved + 32;
	saves[pri].params = sresize(saves[pri].params, saves[pri].savesize,
				    struct cmdline_saved_param);
    }
    saves[pri].params[saves[pri].nsaved].p = p;
    saves[pri].params[saves[pri].nsaved].value = value;
    saves[pri].nsaved++;
}

static char *cmdline_password = NULL;

void cmdline_cleanup(void)
{
    int pri;

    if (cmdline_password) {
	smemclr(cmdline_password, strlen(cmdline_password));
	sfree(cmdline_password);
	cmdline_password = NULL;
    }
    
    for (pri = 0; pri < NPRIORITIES; pri++) {
	sfree(saves[pri].params);
	saves[pri].params = NULL;
	saves[pri].savesize = 0;
	saves[pri].nsaved = 0;
    }
}

#define SAVEABLE(pri) do { \
    if (need_save) { cmdline_save_param(p, value, pri); return ret; } \
} while (0)

/*
 * Similar interface to get_userpass_input(), except that here a -1
 * return means that we aren't capable of processing the prompt and
 * someone else should do it.
 */
int cmdline_get_passwd_input(prompts_t *p, unsigned char *in, int inlen) {

    static int tried_once = 0;

    /*
     * We only handle prompts which don't echo (which we assume to be
     * passwords), and (currently) we only cope with a password prompt
     * that comes in a prompt-set on its own.
     */
    if (!cmdline_password || in || p->n_prompts != 1 || p->prompts[0]->echo) {
	return -1;
    }

    /*
     * If we've tried once, return utter failure (no more passwords left
     * to try).
     */
    if (tried_once)
	return 0;

    prompt_set_result(p->prompts[0], cmdline_password);
    smemclr(cmdline_password, strlen(cmdline_password));
    sfree(cmdline_password);
    cmdline_password = NULL;
    tried_once = 1;
    return 1;
}

/*
 * Here we have a flags word which describes the capabilities of
 * the particular tool on whose behalf we're running. We will
 * refuse certain command-line options if a particular tool
 * inherently can't do anything sensible. For example, the file
 * transfer tools (psftp, pscp) can't do a great deal with protocol
 * selections (ever tried running scp over telnet?) or with port
 * forwarding (even if it wasn't a hideously bad idea, they don't
 * have the select() infrastructure to make them work).
 */
int cmdline_tooltype = 0;

static int cmdline_check_unavailable(int flag, char *p)
{
    if (cmdline_tooltype & flag) {
	cmdline_error("option \"%s\" not available in this tool", p);
	return 1;
    }
    return 0;
}

#define UNAVAILABLE_IN(flag) do { \
    if (cmdline_check_unavailable(flag, p)) return ret; \
} while (0)

/*
 * Process a standard command-line parameter. `p' is the parameter
 * in question; `value' is the subsequent element of argv, which
 * may or may not be required as an operand to the parameter.
 * If `need_save' is 1, arguments which need to be saved as
 * described at this top of this file are, for later execution;
 * if 0, they are processed normally. (-1 is a special value used
 * by pterm to count arguments for a preliminary pass through the
 * argument list; it causes immediate return with an appropriate
 * value with no action taken.)
 * Return value is 2 if both arguments were used; 1 if only p was
 * used; 0 if the parameter wasn't one we recognised; -2 if it
 * should have been 2 but value was NULL.
 */

#define RETURN(x) do { \
    if ((x) == 2 && !value) return -2; \
    ret = x; \
    if (need_save < 0) return x; \
} while (0)

int cmdline_process_param(char *p, char *value, int need_save, Conf *conf)
{
    int ret = 0;

    if (!strcmp(p, "-load")) {
	RETURN(2);
	/* This parameter must be processed immediately rather than being
	 * saved. */
	do_defaults(value, conf);
	loaded_session = TRUE;
	cmdline_session_name = dupstr(value);
	return 2;
    }
    if (!strcmp(p, "-ssh")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	default_protocol = PROT_SSH;
	default_port = 22;
	conf_set_int(conf, CONF_protocol, default_protocol);
	conf_set_int(conf, CONF_port, default_port);
	return 1;
    }
    if (!strcmp(p, "-telnet")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	default_protocol = PROT_TELNET;
	default_port = 23;
	conf_set_int(conf, CONF_protocol, default_protocol);
	conf_set_int(conf, CONF_port, default_port);
	return 1;
    }
    if (!strcmp(p, "-rlogin")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	default_protocol = PROT_RLOGIN;
	default_port = 513;
	conf_set_int(conf, CONF_protocol, default_protocol);
	conf_set_int(conf, CONF_port, default_port);
	return 1;
    }
    if (!strcmp(p, "-raw")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	default_protocol = PROT_RAW;
	conf_set_int(conf, CONF_protocol, default_protocol);
    }
    if (!strcmp(p, "-serial")) {
	RETURN(1);
	/* Serial is not NONNETWORK in an odd sense of the word */
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	default_protocol = PROT_SERIAL;
	conf_set_int(conf, CONF_protocol, default_protocol);
	/* The host parameter will already be loaded into CONF_host,
	 * so copy it across */
	conf_set_str(conf, CONF_serline, conf_get_str(conf, CONF_host));
    }
    if (!strcmp(p, "-v")) {
	RETURN(1);
	flags |= FLAG_VERBOSE;
    }
    if (!strcmp(p, "-l")) {
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_str(conf, CONF_username, value);
    }
    if (!strcmp(p, "-loghost")) {
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_str(conf, CONF_loghost, value);
    }
    if (!strcmp(p, "-hostkey")) {
        char *dup;
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
        dup = dupstr(value);
        if (!validate_manual_hostkey(dup)) {
            cmdline_error("'%s' is not a valid format for a manual host "
                          "key specification", value);
            sfree(dup);
            return ret;
        }
	conf_set_str_str(conf, CONF_ssh_manual_hostkeys, dup, "");
        sfree(dup);
    }
    if ((!strcmp(p, "-L") || !strcmp(p, "-R") || !strcmp(p, "-D"))) {
	char type, *q, *qq, *key, *val;
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	if (strcmp(p, "-D")) {
	    /*
             * For -L or -R forwarding types:
             *
	     * We expect _at least_ two colons in this string. The
	     * possible formats are `sourceport:desthost:destport',
	     * or `sourceip:sourceport:desthost:destport' if you're
	     * specifying a particular loopback address. We need to
	     * replace the one between source and dest with a \t;
	     * this means we must find the second-to-last colon in
	     * the string.
	     *
	     * (This looks like a foolish way of doing it given the
	     * existence of strrchr, but it's more efficient than
	     * two strrchrs - not to mention that the second strrchr
	     * would require us to modify the input string!)
	     */

            type = p[1];               /* 'L' or 'R' */

	    q = qq = host_strchr(value, ':');
	    while (qq) {
		char *qqq = host_strchr(qq+1, ':');
		if (qqq)
		    q = qq;
		qq = qqq;
	    }

	    if (!q) {
		cmdline_error("-%c expects at least two colons in its"
			      " argument", type);
		return ret;
	    }

	    key = dupprintf("%c%.*s", type, (int)(q - value), value);
	    val = dupstr(q+1);
	} else {
            /*
             * Dynamic port forwardings are entered under the same key
             * as if they were local (because they occupy the same
             * port space - a local and a dynamic forwarding on the
             * same local port are mutually exclusive), with the
             * special value "D" (which can be distinguished from
             * anything in the ordinary -L case by containing no
             * colon).
             */
	    key = dupprintf("L%s", value);
	    val = dupstr("D");
	}
	conf_set_str_str(conf, CONF_portfwd, key, val);
	sfree(key);
	sfree(val);
    }
    if ((!strcmp(p, "-nc"))) {
	char *host, *portp;

	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);

	portp = host_strchr(value, ':');
	if (!portp) {
	    cmdline_error("-nc expects argument of form 'host:port'");
	    return ret;
	}

	host = dupprintf("%.*s", (int)(portp - value), value);
	conf_set_str(conf, CONF_ssh_nc_host, host);
	conf_set_int(conf, CONF_ssh_nc_port, atoi(portp + 1));
        sfree(host);
    }
    if (!strcmp(p, "-m")) {
	char *filename, *command;
	int cmdlen, cmdsize;
	FILE *fp;
	int c, d;

	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);

	filename = value;

	cmdlen = cmdsize = 0;
	command = NULL;
	fp = fopen(filename, "r");
	if (!fp) {
	    cmdline_error("unable to open command file \"%s\"", filename);
	    return ret;
	}
	do {
	    c = fgetc(fp);
	    d = c;
	    if (c == EOF)
		d = 0;
	    if (cmdlen >= cmdsize) {
		cmdsize = cmdlen + 512;
		command = sresize(command, cmdsize, char);
	    }
	    command[cmdlen++] = d;
	} while (c != EOF);
	fclose(fp);
	conf_set_str(conf, CONF_remote_cmd, command);
	conf_set_str(conf, CONF_remote_cmd2, "");
	conf_set_int(conf, CONF_nopty, TRUE);   /* command => no terminal */
	sfree(command);
    }
    if (!strcmp(p, "-P")) {
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(1);		       /* lower priority than -ssh,-telnet */
	conf_set_int(conf, CONF_port, atoi(value));
    }
    if (!strcmp(p, "-pw")) {
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(1);
	/* We delay evaluating this until after the protocol is decided,
	 * so that we can warn if it's of no use with the selected protocol */
	if (conf_get_int(conf, CONF_protocol) != PROT_SSH)
	    cmdline_error("the -pw option can only be used with the "
			  "SSH protocol");
	else {
	    cmdline_password = dupstr(value);
	    /* Assuming that `value' is directly from argv, make a good faith
	     * attempt to trample it, to stop it showing up in `ps' output
	     * on Unix-like systems. Not guaranteed, of course. */
	    smemclr(value, strlen(value));
	}
    }

    if (!strcmp(p, "-agent") || !strcmp(p, "-pagent") ||
	!strcmp(p, "-pageant")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_tryagent, TRUE);
    }
    if (!strcmp(p, "-noagent") || !strcmp(p, "-nopagent") ||
	!strcmp(p, "-nopageant")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_tryagent, FALSE);
    }

    if (!strcmp(p, "-A")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_agentfwd, 1);
    }
    if (!strcmp(p, "-a")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_agentfwd, 0);
    }

    if (!strcmp(p, "-X")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_x11_forward, 1);
    }
    if (!strcmp(p, "-x")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_x11_forward, 0);
    }

    if (!strcmp(p, "-t")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(1);	/* lower priority than -m */
	conf_set_int(conf, CONF_nopty, 0);
    }
    if (!strcmp(p, "-T")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(1);
	conf_set_int(conf, CONF_nopty, 1);
    }

    if (!strcmp(p, "-N")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_ssh_no_shell, 1);
    }

    if (!strcmp(p, "-C")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_compression, 1);
    }

    if (!strcmp(p, "-1")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_sshprot, 0);   /* ssh protocol 1 only */
    }
    if (!strcmp(p, "-2")) {
	RETURN(1);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	conf_set_int(conf, CONF_sshprot, 3);   /* ssh protocol 2 only */
    }

    if (!strcmp(p, "-i")) {
	Filename *fn;
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	fn = filename_from_str(value);
	conf_set_filename(conf, CONF_keyfile, fn);
        filename_free(fn);
    }

    if (!strcmp(p, "-4") || !strcmp(p, "-ipv4")) {
	RETURN(1);
	SAVEABLE(1);
	conf_set_int(conf, CONF_addressfamily, ADDRTYPE_IPV4);
    }
    if (!strcmp(p, "-6") || !strcmp(p, "-ipv6")) {
	RETURN(1);
	SAVEABLE(1);
	conf_set_int(conf, CONF_addressfamily, ADDRTYPE_IPV6);
    }
    if (!strcmp(p, "-sercfg")) {
	char* nextitem;
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER | TOOLTYPE_NONNETWORK);
	SAVEABLE(1);
	if (conf_get_int(conf, CONF_protocol) != PROT_SERIAL)
	    cmdline_error("the -sercfg option can only be used with the "
			  "serial protocol");
	/* Value[0] contains one or more , separated values, like 19200,8,n,1,X */
	nextitem = value;
	while (nextitem[0] != '\0') {
	    int length, skip;
	    char *end = strchr(nextitem, ',');
	    if (!end) {
		length = strlen(nextitem);
		skip = 0;
	    } else {
		length = end - nextitem;
		nextitem[length] = '\0';
		skip = 1;
	    }
	    if (length == 1) {
		switch (*nextitem) {
		  case '1':
		  case '2':
		    conf_set_int(conf, CONF_serstopbits, 2 * (*nextitem-'0'));
		    break;

		  case '5':
		  case '6':
		  case '7':
		  case '8':
		  case '9':
		    conf_set_int(conf, CONF_serdatabits, *nextitem-'0');
		    break;

		  case 'n':
		    conf_set_int(conf, CONF_serparity, SER_PAR_NONE);
		    break;
		  case 'o':
		    conf_set_int(conf, CONF_serparity, SER_PAR_ODD);
		    break;
		  case 'e':
		    conf_set_int(conf, CONF_serparity, SER_PAR_EVEN);
		    break;
		  case 'm':
		    conf_set_int(conf, CONF_serparity, SER_PAR_MARK);
		    break;
		  case 's':
		    conf_set_int(conf, CONF_serparity, SER_PAR_SPACE);
		    break;

		  case 'N':
		    conf_set_int(conf, CONF_serflow, SER_FLOW_NONE);
		    break;
		  case 'X':
		    conf_set_int(conf, CONF_serflow, SER_FLOW_XONXOFF);
		    break;
		  case 'R':
		    conf_set_int(conf, CONF_serflow, SER_FLOW_RTSCTS);
		    break;
		  case 'D':
		    conf_set_int(conf, CONF_serflow, SER_FLOW_DSRDTR);
		    break;

		  default:
		    cmdline_error("Unrecognised suboption \"-sercfg %c\"",
				  *nextitem);
		}
	    } else if (length == 3 && !strncmp(nextitem,"1.5",3)) {
		/* Messy special case */
		conf_set_int(conf, CONF_serstopbits, 3);
	    } else {
		int serspeed = atoi(nextitem);
		if (serspeed != 0) {
		    conf_set_int(conf, CONF_serspeed, serspeed);
		} else {
		    cmdline_error("Unrecognised suboption \"-sercfg %s\"",
				  nextitem);
		}
	    }
	    nextitem += length + skip;
	}
    }

    if (!strcmp(p, "-sessionlog")) {
	Filename *fn;
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_FILETRANSFER);
	/* but available even in TOOLTYPE_NONNETWORK, cf pterm "-log" */
	SAVEABLE(0);
	fn = filename_from_str(value);
	conf_set_filename(conf, CONF_logfilename, fn);
	conf_set_int(conf, CONF_logtype, LGTYP_DEBUG);
        filename_free(fn);
    }

    if (!strcmp(p, "-sshlog") ||
        !strcmp(p, "-sshrawlog")) {
	Filename *fn;
	RETURN(2);
	UNAVAILABLE_IN(TOOLTYPE_NONNETWORK);
	SAVEABLE(0);
	fn = filename_from_str(value);
	conf_set_filename(conf, CONF_logfilename, fn);
	conf_set_int(conf, CONF_logtype,
                     !strcmp(p, "-sshlog") ? LGTYP_PACKETS :
                     /* !strcmp(p, "-sshrawlog") ? */ LGTYP_SSHRAW);
        filename_free(fn);
    }

    return ret;			       /* unrecognised */
}

void cmdline_run_saved(Conf *conf)
{
    int pri, i;
    for (pri = 0; pri < NPRIORITIES; pri++)
	for (i = 0; i < saves[pri].nsaved; i++)
	    cmdline_process_param(saves[pri].params[i].p,
				  saves[pri].params[i].value, 0, conf);
}
