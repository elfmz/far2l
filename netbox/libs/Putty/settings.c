/*
 * settings.c: read and write saved sessions. (platform-independent)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "putty.h"
#include "storage.h"

/* The cipher order given here is the default order. */
static const struct keyvalwhere ciphernames[] = {
    { "aes",        CIPHER_AES,             -1, -1 },
    { "blowfish",   CIPHER_BLOWFISH,        -1, -1 },
    { "3des",       CIPHER_3DES,            -1, -1 },
    { "WARN",       CIPHER_WARN,            -1, -1 },
    { "arcfour",    CIPHER_ARCFOUR,         -1, -1 },
    { "des",        CIPHER_DES,             -1, -1 }
};

static const struct keyvalwhere kexnames[] = {
    { "dh-gex-sha1",        KEX_DHGEX,      -1, -1 },
    { "dh-group14-sha1",    KEX_DHGROUP14,  -1, -1 },
    { "dh-group1-sha1",     KEX_DHGROUP1,   -1, -1 },
    { "rsa",                KEX_RSA,        KEX_WARN, -1 },
    { "WARN",               KEX_WARN,       -1, -1 }
};

/*
 * All the terminal modes that we know about for the "TerminalModes"
 * setting. (Also used by config.c for the drop-down list.)
 * This is currently precisely the same as the set in ssh.c, but could
 * in principle differ if other backends started to support tty modes
 * (e.g., the pty backend).
 */
const char *const ttymodes[] = {
    "INTR",	"QUIT",     "ERASE",	"KILL",     "EOF",
    "EOL",	"EOL2",     "START",	"STOP",     "SUSP",
    "DSUSP",	"REPRINT",  "WERASE",	"LNEXT",    "FLUSH",
    "SWTCH",	"STATUS",   "DISCARD",	"IGNPAR",   "PARMRK",
    "INPCK",	"ISTRIP",   "INLCR",	"IGNCR",    "ICRNL",
    "IUCLC",	"IXON",     "IXANY",	"IXOFF",    "IMAXBEL",
    "ISIG",	"ICANON",   "XCASE",	"ECHO",     "ECHOE",
    "ECHOK",	"ECHONL",   "NOFLSH",	"TOSTOP",   "IEXTEN",
    "ECHOCTL",	"ECHOKE",   "PENDIN",	"OPOST",    "OLCUC",
    "ONLCR",	"OCRNL",    "ONOCR",	"ONLRET",   "CS7",
    "CS8",	"PARENB",   "PARODD",	NULL
};

/*
 * Convenience functions to access the backends[] array
 * (which is only present in tools that manage settings).
 */

Backend *backend_from_name(const char *name)
{
    Backend **p;
    for (p = backends; *p != NULL; p++)
	if (!strcmp((*p)->name, name))
	    return *p;
    return NULL;
}

Backend *backend_from_proto(int proto)
{
    Backend **p;
    for (p = backends; *p != NULL; p++)
	if ((*p)->protocol == proto)
	    return *p;
    return NULL;
}

char *get_remote_username(Conf *conf)
{
    char *username = conf_get_str(conf, CONF_username);
    if (*username) {
	return dupstr(username);
    } else if (conf_get_int(conf, CONF_username_from_env)) {
	/* Use local username. */
	return get_username();     /* might still be NULL */
    } else {
	return NULL;
    }
}

static char *gpps_raw(void *handle, const char *name, const char *def)
{
    char *ret = read_setting_s(handle, name);
    if (!ret)
	ret = platform_default_s(name);
    if (!ret)
	ret = def ? dupstr(def) : NULL;   /* permit NULL as final fallback */
    return ret;
}

static void gpps(void *handle, const char *name, const char *def,
		 Conf *conf, int primary)
{
    char *val = gpps_raw(handle, name, def);
    conf_set_str(conf, primary, val);
    sfree(val);
}

/*
 * gppfont and gppfile cannot have local defaults, since the very
 * format of a Filename or FontSpec is platform-dependent. So the
 * platform-dependent functions MUST return some sort of value.
 */
static void gppfont(void *handle, const char *name, Conf *conf, int primary)
{
    FontSpec *result = read_setting_fontspec(handle, name);
    if (!result)
        result = platform_default_fontspec(name);
    conf_set_fontspec(conf, primary, result);
    fontspec_free(result);
}
static void gppfile(void *handle, const char *name, Conf *conf, int primary)
{
    Filename *result = read_setting_filename(handle, name);
    if (!result)
	result = platform_default_filename(name);
    conf_set_filename(conf, primary, result);
    filename_free(result);
}

static int gppi_raw(void *handle, char *name, int def)
{
    def = platform_default_i(name, def);
    return read_setting_i(handle, name, def);
}

static void gppi(void *handle, char *name, int def, Conf *conf, int primary)
{
    conf_set_int(conf, primary, gppi_raw(handle, name, def));
}

/*
 * Read a set of name-value pairs in the format we occasionally use:
 *   NAME\tVALUE\0NAME\tVALUE\0\0 in memory
 *   NAME=VALUE,NAME=VALUE, in storage
 * If there's no "=VALUE" (e.g. just NAME,NAME,NAME) then those keys
 * are mapped to the empty string.
 */
static int gppmap(void *handle, char *name, Conf *conf, int primary)
{
    char *buf, *p, *q, *key, *val;

    /*
     * Start by clearing any existing subkeys of this key from conf.
     */
    while ((key = conf_get_str_nthstrkey(conf, primary, 0)) != NULL)
        conf_del_str_str(conf, primary, key);

    /*
     * Now read a serialised list from the settings and unmarshal it
     * into its components.
     */
    buf = gpps_raw(handle, name, NULL);
    if (!buf)
	return FALSE;

    p = buf;
    while (*p) {
	q = buf;
	val = NULL;
	while (*p && *p != ',') {
	    int c = *p++;
	    if (c == '=')
		c = '\0';
	    if (c == '\\')
		c = *p++;
	    *q++ = c;
	    if (!c)
		val = q;
	}
	if (*p == ',')
	    p++;
	if (!val)
	    val = q;
	*q = '\0';

        if (primary == CONF_portfwd && strchr(buf, 'D') != NULL) {
            /*
             * Backwards-compatibility hack: dynamic forwardings are
             * indexed in the data store as a third type letter in the
             * key, 'D' alongside 'L' and 'R' - but really, they
             * should be filed under 'L' with a special _value_,
             * because local and dynamic forwardings both involve
             * _listening_ on a local port, and are hence mutually
             * exclusive on the same port number. So here we translate
             * the legacy storage format into the sensible internal
             * form, by finding the D and turning it into a L.
             */
            char *newkey = dupstr(buf);
            *strchr(newkey, 'D') = 'L';
            conf_set_str_str(conf, primary, newkey, "D");
            sfree(newkey);
        } else {
            conf_set_str_str(conf, primary, buf, val);
        }
    }
    sfree(buf);

    return TRUE;
}

/*
 * Write a set of name/value pairs in the above format, or just the
 * names if include_values is FALSE.
 */
static void wmap(void *handle, char const *outkey, Conf *conf, int primary,
                 int include_values)
{
    char *buf, *p, *q, *key, *realkey, *val;
    int len;

    len = 1;			       /* allow for NUL */

    for (val = conf_get_str_strs(conf, primary, NULL, &key);
	 val != NULL;
	 val = conf_get_str_strs(conf, primary, key, &key))
	len += 2 + 2 * (strlen(key) + strlen(val));   /* allow for escaping */

    buf = snewn(len, char);
    p = buf;

    for (val = conf_get_str_strs(conf, primary, NULL, &key);
	 val != NULL;
	 val = conf_get_str_strs(conf, primary, key, &key)) {

        if (primary == CONF_portfwd && !strcmp(val, "D")) {
            /*
             * Backwards-compatibility hack, as above: translate from
             * the sensible internal representation of dynamic
             * forwardings (key "L<port>", value "D") to the
             * conceptually incoherent legacy storage format (key
             * "D<port>", value empty).
             */
            char *L;

            realkey = key;             /* restore it at end of loop */
            val = "";
            key = dupstr(key);
            L = strchr(key, 'L');
            if (L) *L = 'D';
        } else {
            realkey = NULL;
        }

	if (p != buf)
	    *p++ = ',';
	for (q = key; *q; q++) {
	    if (*q == '=' || *q == ',' || *q == '\\')
		*p++ = '\\';
	    *p++ = *q;
	}
        if (include_values) {
            *p++ = '=';
            for (q = val; *q; q++) {
                if (*q == '=' || *q == ',' || *q == '\\')
                    *p++ = '\\';
                *p++ = *q;
            }
        }

        if (realkey) {
            free(key);
            key = realkey;
        }
    }
    *p = '\0';
    write_setting_s(handle, outkey, buf);
    sfree(buf);
}

static int key2val(const struct keyvalwhere *mapping,
                   int nmaps, char *key)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (!strcmp(mapping[i].s, key)) return mapping[i].v;
    return -1;
}

static const char *val2key(const struct keyvalwhere *mapping,
                           int nmaps, int val)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (mapping[i].v == val) return mapping[i].s;
    return NULL;
}

/*
 * Helper function to parse a comma-separated list of strings into
 * a preference list array of values. Any missing values are added
 * to the end and duplicates are weeded.
 * XXX: assumes vals in 'mapping' are small +ve integers
 */
static void gprefs(void *sesskey, char *name, char *def,
		   const struct keyvalwhere *mapping, int nvals,
		   Conf *conf, int primary)
{
    char *commalist;
    char *p, *q;
    int i, j, n, v, pos;
    unsigned long seen = 0;	       /* bitmap for weeding dups etc */

    /*
     * Fetch the string which we'll parse as a comma-separated list.
     */
    commalist = gpps_raw(sesskey, name, def);

    /*
     * Go through that list and convert it into values.
     */
    n = 0;
    p = commalist;
    while (1) {
        while (*p && *p == ',') p++;
        if (!*p)
            break;                     /* no more words */

        q = p;
        while (*p && *p != ',') p++;
        if (*p) *p++ = '\0';

        v = key2val(mapping, nvals, q);
        if (v != -1 && !(seen & (1 << v))) {
	    seen |= (1 << v);
            conf_set_int_int(conf, primary, n, v);
            n++;
	}
    }

    sfree(commalist);

    /*
     * Now go through 'mapping' and add values that weren't mentioned
     * in the list we fetched. We may have to loop over it multiple
     * times so that we add values before other values whose default
     * positions depend on them.
     */
    while (n < nvals) {
        for (i = 0; i < nvals; i++) {
	    assert(mapping[i].v < 32);

	    if (!(seen & (1 << mapping[i].v))) {
                /*
                 * This element needs adding. But can we add it yet?
                 */
                if (mapping[i].vrel != -1 && !(seen & (1 << mapping[i].vrel)))
                    continue;          /* nope */

                /*
                 * OK, we can work out where to add this element, so
                 * do so.
                 */
                if (mapping[i].vrel == -1) {
                    pos = (mapping[i].where < 0 ? n : 0);
                } else {
                    for (j = 0; j < n; j++)
                        if (conf_get_int_int(conf, primary, j) ==
                            mapping[i].vrel)
                            break;
                    assert(j < n);     /* implied by (seen & (1<<vrel)) */
                    pos = (mapping[i].where < 0 ? j : j+1);
                }

                /*
                 * And add it.
                 */
                for (j = n-1; j >= pos; j--)
                    conf_set_int_int(conf, primary, j+1,
                                     conf_get_int_int(conf, primary, j));
                conf_set_int_int(conf, primary, pos, mapping[i].v);
                n++;
            }
        }
    }
}

/* 
 * Write out a preference list.
 */
static void wprefs(void *sesskey, char *name,
		   const struct keyvalwhere *mapping, int nvals,
		   Conf *conf, int primary)
{
    char *buf, *p;
    int i, maxlen;

    for (maxlen = i = 0; i < nvals; i++) {
	const char *s = val2key(mapping, nvals,
                                conf_get_int_int(conf, primary, i));
	if (s) {
            maxlen += (maxlen > 0 ? 1 : 0) + strlen(s);
        }
    }

    buf = snewn(maxlen + 1, char);
    p = buf;

    for (i = 0; i < nvals; i++) {
	const char *s = val2key(mapping, nvals,
                                conf_get_int_int(conf, primary, i));
	if (s) {
            p += sprintf(p, "%s%s", (p > buf ? "," : ""), s);
	}
    }

    assert(p - buf == maxlen);
    *p = '\0';

    write_setting_s(sesskey, name, buf);

    sfree(buf);
}

char *save_settings(char *section, Conf *conf)
{
    void *sesskey;
    char *errmsg;

    sesskey = open_settings_w(section, &errmsg);
    if (!sesskey)
	return errmsg;
    save_open_settings(sesskey, conf);
    close_settings_w(sesskey);
    return NULL;
}

void save_open_settings(void *sesskey, Conf *conf)
{
    int i;
    char *p;

    write_setting_i(sesskey, "Present", 1);
    write_setting_s(sesskey, "HostName", conf_get_str(conf, CONF_host));
    write_setting_filename(sesskey, "LogFileName", conf_get_filename(conf, CONF_logfilename));
    write_setting_i(sesskey, "LogType", conf_get_int(conf, CONF_logtype));
    write_setting_i(sesskey, "LogFileClash", conf_get_int(conf, CONF_logxfovr));
    write_setting_i(sesskey, "LogFlush", conf_get_int(conf, CONF_logflush));
    write_setting_i(sesskey, "SSHLogOmitPasswords", conf_get_int(conf, CONF_logomitpass));
    write_setting_i(sesskey, "SSHLogOmitData", conf_get_int(conf, CONF_logomitdata));
    p = "raw";
    {
	const Backend *b = backend_from_proto(conf_get_int(conf, CONF_protocol));
	if (b)
	    p = b->name;
    }
    write_setting_s(sesskey, "Protocol", p);
    write_setting_i(sesskey, "PortNumber", conf_get_int(conf, CONF_port));
    /* The CloseOnExit numbers are arranged in a different order from
     * the standard FORCE_ON / FORCE_OFF / AUTO. */
    write_setting_i(sesskey, "CloseOnExit", (conf_get_int(conf, CONF_close_on_exit)+2)%3);
    write_setting_i(sesskey, "WarnOnClose", !!conf_get_int(conf, CONF_warn_on_close));
    write_setting_i(sesskey, "PingInterval", conf_get_int(conf, CONF_ping_interval) / 60);	/* minutes */
    write_setting_i(sesskey, "PingIntervalSecs", conf_get_int(conf, CONF_ping_interval) % 60);	/* seconds */
    write_setting_i(sesskey, "TCPNoDelay", conf_get_int(conf, CONF_tcp_nodelay));
    write_setting_i(sesskey, "TCPKeepalives", conf_get_int(conf, CONF_tcp_keepalives));
    write_setting_s(sesskey, "TerminalType", conf_get_str(conf, CONF_termtype));
    write_setting_s(sesskey, "TerminalSpeed", conf_get_str(conf, CONF_termspeed));
    wmap(sesskey, "TerminalModes", conf, CONF_ttymodes, TRUE);

    /* Address family selection */
    write_setting_i(sesskey, "AddressFamily", conf_get_int(conf, CONF_addressfamily));

    /* proxy settings */
    write_setting_s(sesskey, "ProxyExcludeList", conf_get_str(conf, CONF_proxy_exclude_list));
    write_setting_i(sesskey, "ProxyDNS", (conf_get_int(conf, CONF_proxy_dns)+2)%3);
    write_setting_i(sesskey, "ProxyLocalhost", conf_get_int(conf, CONF_even_proxy_localhost));
    write_setting_i(sesskey, "ProxyMethod", conf_get_int(conf, CONF_proxy_type));
    write_setting_s(sesskey, "ProxyHost", conf_get_str(conf, CONF_proxy_host));
    write_setting_i(sesskey, "ProxyPort", conf_get_int(conf, CONF_proxy_port));
    write_setting_s(sesskey, "ProxyUsername", conf_get_str(conf, CONF_proxy_username));
    write_setting_s(sesskey, "ProxyPassword", conf_get_str(conf, CONF_proxy_password));
    write_setting_s(sesskey, "ProxyTelnetCommand", conf_get_str(conf, CONF_proxy_telnet_command));
    wmap(sesskey, "Environment", conf, CONF_environmt, TRUE);
    write_setting_s(sesskey, "UserName", conf_get_str(conf, CONF_username));
    write_setting_i(sesskey, "UserNameFromEnvironment", conf_get_int(conf, CONF_username_from_env));
    write_setting_s(sesskey, "LocalUserName", conf_get_str(conf, CONF_localusername));
    write_setting_i(sesskey, "NoPTY", conf_get_int(conf, CONF_nopty));
    write_setting_i(sesskey, "Compression", conf_get_int(conf, CONF_compression));
    write_setting_i(sesskey, "TryAgent", conf_get_int(conf, CONF_tryagent));
    write_setting_i(sesskey, "AgentFwd", conf_get_int(conf, CONF_agentfwd));
    write_setting_i(sesskey, "GssapiFwd", conf_get_int(conf, CONF_gssapifwd));
    write_setting_i(sesskey, "ChangeUsername", conf_get_int(conf, CONF_change_username));
    wprefs(sesskey, "Cipher", ciphernames, CIPHER_MAX, conf, CONF_ssh_cipherlist);
    wprefs(sesskey, "KEX", kexnames, KEX_MAX, conf, CONF_ssh_kexlist);
    write_setting_i(sesskey, "RekeyTime", conf_get_int(conf, CONF_ssh_rekey_time));
    write_setting_s(sesskey, "RekeyBytes", conf_get_str(conf, CONF_ssh_rekey_data));
    write_setting_i(sesskey, "SshNoAuth", conf_get_int(conf, CONF_ssh_no_userauth));
    write_setting_i(sesskey, "SshBanner", conf_get_int(conf, CONF_ssh_show_banner));
    write_setting_i(sesskey, "AuthTIS", conf_get_int(conf, CONF_try_tis_auth));
    write_setting_i(sesskey, "AuthKI", conf_get_int(conf, CONF_try_ki_auth));
    write_setting_i(sesskey, "AuthGSSAPI", conf_get_int(conf, CONF_try_gssapi_auth));
#ifndef NO_GSSAPI
    wprefs(sesskey, "GSSLibs", gsslibkeywords, ngsslibs, conf, CONF_ssh_gsslist);
    write_setting_filename(sesskey, "GSSCustom", conf_get_filename(conf, CONF_ssh_gss_custom));
#endif
    write_setting_i(sesskey, "SshNoShell", conf_get_int(conf, CONF_ssh_no_shell));
    write_setting_i(sesskey, "SshProt", conf_get_int(conf, CONF_sshprot));
    write_setting_s(sesskey, "LogHost", conf_get_str(conf, CONF_loghost));
    write_setting_i(sesskey, "SSH2DES", conf_get_int(conf, CONF_ssh2_des_cbc));
    write_setting_filename(sesskey, "PublicKeyFile", conf_get_filename(conf, CONF_keyfile));
    write_setting_s(sesskey, "RemoteCommand", conf_get_str(conf, CONF_remote_cmd));
    write_setting_i(sesskey, "RFCEnviron", conf_get_int(conf, CONF_rfc_environ));
    write_setting_i(sesskey, "PassiveTelnet", conf_get_int(conf, CONF_passive_telnet));
    write_setting_i(sesskey, "BackspaceIsDelete", conf_get_int(conf, CONF_bksp_is_delete));
    write_setting_i(sesskey, "RXVTHomeEnd", conf_get_int(conf, CONF_rxvt_homeend));
    write_setting_i(sesskey, "LinuxFunctionKeys", conf_get_int(conf, CONF_funky_type));
    write_setting_i(sesskey, "NoApplicationKeys", conf_get_int(conf, CONF_no_applic_k));
    write_setting_i(sesskey, "NoApplicationCursors", conf_get_int(conf, CONF_no_applic_c));
    write_setting_i(sesskey, "NoMouseReporting", conf_get_int(conf, CONF_no_mouse_rep));
    write_setting_i(sesskey, "NoRemoteResize", conf_get_int(conf, CONF_no_remote_resize));
    write_setting_i(sesskey, "NoAltScreen", conf_get_int(conf, CONF_no_alt_screen));
    write_setting_i(sesskey, "NoRemoteWinTitle", conf_get_int(conf, CONF_no_remote_wintitle));
    write_setting_i(sesskey, "RemoteQTitleAction", conf_get_int(conf, CONF_remote_qtitle_action));
    write_setting_i(sesskey, "NoDBackspace", conf_get_int(conf, CONF_no_dbackspace));
    write_setting_i(sesskey, "NoRemoteCharset", conf_get_int(conf, CONF_no_remote_charset));
    write_setting_i(sesskey, "ApplicationCursorKeys", conf_get_int(conf, CONF_app_cursor));
    write_setting_i(sesskey, "ApplicationKeypad", conf_get_int(conf, CONF_app_keypad));
    write_setting_i(sesskey, "NetHackKeypad", conf_get_int(conf, CONF_nethack_keypad));
    write_setting_i(sesskey, "AltF4", conf_get_int(conf, CONF_alt_f4));
    write_setting_i(sesskey, "AltSpace", conf_get_int(conf, CONF_alt_space));
    write_setting_i(sesskey, "AltOnly", conf_get_int(conf, CONF_alt_only));
    write_setting_i(sesskey, "ComposeKey", conf_get_int(conf, CONF_compose_key));
    write_setting_i(sesskey, "CtrlAltKeys", conf_get_int(conf, CONF_ctrlaltkeys));
    write_setting_i(sesskey, "TelnetKey", conf_get_int(conf, CONF_telnet_keyboard));
    write_setting_i(sesskey, "TelnetRet", conf_get_int(conf, CONF_telnet_newline));
    write_setting_i(sesskey, "LocalEcho", conf_get_int(conf, CONF_localecho));
    write_setting_i(sesskey, "LocalEdit", conf_get_int(conf, CONF_localedit));
    write_setting_s(sesskey, "Answerback", conf_get_str(conf, CONF_answerback));
    write_setting_i(sesskey, "AlwaysOnTop", conf_get_int(conf, CONF_alwaysontop));
    write_setting_i(sesskey, "FullScreenOnAltEnter", conf_get_int(conf, CONF_fullscreenonaltenter));
    write_setting_i(sesskey, "HideMousePtr", conf_get_int(conf, CONF_hide_mouseptr));
    write_setting_i(sesskey, "SunkenEdge", conf_get_int(conf, CONF_sunken_edge));
    write_setting_i(sesskey, "WindowBorder", conf_get_int(conf, CONF_window_border));
    write_setting_i(sesskey, "CurType", conf_get_int(conf, CONF_cursor_type));
    write_setting_i(sesskey, "BlinkCur", conf_get_int(conf, CONF_blink_cur));
    write_setting_i(sesskey, "Beep", conf_get_int(conf, CONF_beep));
    write_setting_i(sesskey, "BeepInd", conf_get_int(conf, CONF_beep_ind));
    write_setting_filename(sesskey, "BellWaveFile", conf_get_filename(conf, CONF_bell_wavefile));
    write_setting_i(sesskey, "BellOverload", conf_get_int(conf, CONF_bellovl));
    write_setting_i(sesskey, "BellOverloadN", conf_get_int(conf, CONF_bellovl_n));
    write_setting_i(sesskey, "BellOverloadT", conf_get_int(conf, CONF_bellovl_t)
#ifdef PUTTY_UNIX_H
		    * 1000
#endif
		    );
    write_setting_i(sesskey, "BellOverloadS", conf_get_int(conf, CONF_bellovl_s)
#ifdef PUTTY_UNIX_H
		    * 1000
#endif
		    );
    write_setting_i(sesskey, "ScrollbackLines", conf_get_int(conf, CONF_savelines));
    write_setting_i(sesskey, "DECOriginMode", conf_get_int(conf, CONF_dec_om));
    write_setting_i(sesskey, "AutoWrapMode", conf_get_int(conf, CONF_wrap_mode));
    write_setting_i(sesskey, "LFImpliesCR", conf_get_int(conf, CONF_lfhascr));
    write_setting_i(sesskey, "CRImpliesLF", conf_get_int(conf, CONF_crhaslf));
    write_setting_i(sesskey, "DisableArabicShaping", conf_get_int(conf, CONF_arabicshaping));
    write_setting_i(sesskey, "DisableBidi", conf_get_int(conf, CONF_bidi));
    write_setting_i(sesskey, "WinNameAlways", conf_get_int(conf, CONF_win_name_always));
    write_setting_s(sesskey, "WinTitle", conf_get_str(conf, CONF_wintitle));
    write_setting_i(sesskey, "TermWidth", conf_get_int(conf, CONF_width));
    write_setting_i(sesskey, "TermHeight", conf_get_int(conf, CONF_height));
    write_setting_fontspec(sesskey, "Font", conf_get_fontspec(conf, CONF_font));
    write_setting_i(sesskey, "FontQuality", conf_get_int(conf, CONF_font_quality));
    write_setting_i(sesskey, "FontVTMode", conf_get_int(conf, CONF_vtmode));
    write_setting_i(sesskey, "UseSystemColours", conf_get_int(conf, CONF_system_colour));
    write_setting_i(sesskey, "TryPalette", conf_get_int(conf, CONF_try_palette));
    write_setting_i(sesskey, "ANSIColour", conf_get_int(conf, CONF_ansi_colour));
    write_setting_i(sesskey, "Xterm256Colour", conf_get_int(conf, CONF_xterm_256_colour));
    write_setting_i(sesskey, "BoldAsColour", conf_get_int(conf, CONF_bold_style)-1);

    for (i = 0; i < 22; i++) {
	char buf[20], buf2[30];
	sprintf(buf, "Colour%d", i);
	sprintf(buf2, "%d,%d,%d",
		conf_get_int_int(conf, CONF_colours, i*3+0),
		conf_get_int_int(conf, CONF_colours, i*3+1),
		conf_get_int_int(conf, CONF_colours, i*3+2));
	write_setting_s(sesskey, buf, buf2);
    }
    write_setting_i(sesskey, "RawCNP", conf_get_int(conf, CONF_rawcnp));
    write_setting_i(sesskey, "PasteRTF", conf_get_int(conf, CONF_rtf_paste));
    write_setting_i(sesskey, "MouseIsXterm", conf_get_int(conf, CONF_mouse_is_xterm));
    write_setting_i(sesskey, "RectSelect", conf_get_int(conf, CONF_rect_select));
    write_setting_i(sesskey, "MouseOverride", conf_get_int(conf, CONF_mouse_override));
    for (i = 0; i < 256; i += 32) {
	char buf[20], buf2[256];
	int j;
	sprintf(buf, "Wordness%d", i);
	*buf2 = '\0';
	for (j = i; j < i + 32; j++) {
	    sprintf(buf2 + strlen(buf2), "%s%d",
		    (*buf2 ? "," : ""),
		    conf_get_int_int(conf, CONF_wordness, j));
	}
	write_setting_s(sesskey, buf, buf2);
    }
    write_setting_s(sesskey, "LineCodePage", conf_get_str(conf, CONF_line_codepage));
    write_setting_i(sesskey, "CJKAmbigWide", conf_get_int(conf, CONF_cjk_ambig_wide));
    write_setting_i(sesskey, "UTF8Override", conf_get_int(conf, CONF_utf8_override));
    write_setting_s(sesskey, "Printer", conf_get_str(conf, CONF_printer));
    write_setting_i(sesskey, "CapsLockCyr", conf_get_int(conf, CONF_xlat_capslockcyr));
    write_setting_i(sesskey, "ScrollBar", conf_get_int(conf, CONF_scrollbar));
    write_setting_i(sesskey, "ScrollBarFullScreen", conf_get_int(conf, CONF_scrollbar_in_fullscreen));
    write_setting_i(sesskey, "ScrollOnKey", conf_get_int(conf, CONF_scroll_on_key));
    write_setting_i(sesskey, "ScrollOnDisp", conf_get_int(conf, CONF_scroll_on_disp));
    write_setting_i(sesskey, "EraseToScrollback", conf_get_int(conf, CONF_erase_to_scrollback));
    write_setting_i(sesskey, "LockSize", conf_get_int(conf, CONF_resize_action));
    write_setting_i(sesskey, "BCE", conf_get_int(conf, CONF_bce));
    write_setting_i(sesskey, "BlinkText", conf_get_int(conf, CONF_blinktext));
    write_setting_i(sesskey, "X11Forward", conf_get_int(conf, CONF_x11_forward));
    write_setting_s(sesskey, "X11Display", conf_get_str(conf, CONF_x11_display));
    write_setting_i(sesskey, "X11AuthType", conf_get_int(conf, CONF_x11_auth));
    write_setting_filename(sesskey, "X11AuthFile", conf_get_filename(conf, CONF_xauthfile));
    write_setting_i(sesskey, "LocalPortAcceptAll", conf_get_int(conf, CONF_lport_acceptall));
    write_setting_i(sesskey, "RemotePortAcceptAll", conf_get_int(conf, CONF_rport_acceptall));
    wmap(sesskey, "PortForwardings", conf, CONF_portfwd, TRUE);
    write_setting_i(sesskey, "BugIgnore1", 2-conf_get_int(conf, CONF_sshbug_ignore1));
    write_setting_i(sesskey, "BugPlainPW1", 2-conf_get_int(conf, CONF_sshbug_plainpw1));
    write_setting_i(sesskey, "BugRSA1", 2-conf_get_int(conf, CONF_sshbug_rsa1));
    write_setting_i(sesskey, "BugIgnore2", 2-conf_get_int(conf, CONF_sshbug_ignore2));
    write_setting_i(sesskey, "BugHMAC2", 2-conf_get_int(conf, CONF_sshbug_hmac2));
    write_setting_i(sesskey, "BugDeriveKey2", 2-conf_get_int(conf, CONF_sshbug_derivekey2));
    write_setting_i(sesskey, "BugRSAPad2", 2-conf_get_int(conf, CONF_sshbug_rsapad2));
    write_setting_i(sesskey, "BugPKSessID2", 2-conf_get_int(conf, CONF_sshbug_pksessid2));
    write_setting_i(sesskey, "BugRekey2", 2-conf_get_int(conf, CONF_sshbug_rekey2));
    write_setting_i(sesskey, "BugMaxPkt2", 2-conf_get_int(conf, CONF_sshbug_maxpkt2));
    write_setting_i(sesskey, "BugOldGex2", 2-conf_get_int(conf, CONF_sshbug_oldgex2));
    write_setting_i(sesskey, "BugWinadj", 2-conf_get_int(conf, CONF_sshbug_winadj));
    write_setting_i(sesskey, "BugChanReq", 2-conf_get_int(conf, CONF_sshbug_chanreq));
    write_setting_i(sesskey, "StampUtmp", conf_get_int(conf, CONF_stamp_utmp));
    write_setting_i(sesskey, "LoginShell", conf_get_int(conf, CONF_login_shell));
    write_setting_i(sesskey, "ScrollbarOnLeft", conf_get_int(conf, CONF_scrollbar_on_left));
    write_setting_fontspec(sesskey, "BoldFont", conf_get_fontspec(conf, CONF_boldfont));
    write_setting_fontspec(sesskey, "WideFont", conf_get_fontspec(conf, CONF_widefont));
    write_setting_fontspec(sesskey, "WideBoldFont", conf_get_fontspec(conf, CONF_wideboldfont));
    write_setting_i(sesskey, "ShadowBold", conf_get_int(conf, CONF_shadowbold));
    write_setting_i(sesskey, "ShadowBoldOffset", conf_get_int(conf, CONF_shadowboldoffset));
    write_setting_s(sesskey, "SerialLine", conf_get_str(conf, CONF_serline));
    write_setting_i(sesskey, "SerialSpeed", conf_get_int(conf, CONF_serspeed));
    write_setting_i(sesskey, "SerialDataBits", conf_get_int(conf, CONF_serdatabits));
    write_setting_i(sesskey, "SerialStopHalfbits", conf_get_int(conf, CONF_serstopbits));
    write_setting_i(sesskey, "SerialParity", conf_get_int(conf, CONF_serparity));
    write_setting_i(sesskey, "SerialFlowControl", conf_get_int(conf, CONF_serflow));
    write_setting_s(sesskey, "WindowClass", conf_get_str(conf, CONF_winclass));
    write_setting_i(sesskey, "ConnectionSharing", conf_get_int(conf, CONF_ssh_connection_sharing));
    write_setting_i(sesskey, "ConnectionSharingUpstream", conf_get_int(conf, CONF_ssh_connection_sharing_upstream));
    write_setting_i(sesskey, "ConnectionSharingDownstream", conf_get_int(conf, CONF_ssh_connection_sharing_downstream));
    wmap(sesskey, "SSHManualHostKeys", conf, CONF_ssh_manual_hostkeys, FALSE);
}

void load_settings(char *section, Conf *conf)
{
    void *sesskey;

    sesskey = open_settings_r(section);
    load_open_settings(sesskey, conf);
    close_settings_r(sesskey);

    if (conf_launchable(conf))
        add_session_to_jumplist(section);
}

void load_open_settings(void *sesskey, Conf *conf)
{
    int i;
    char *prot;

    conf_set_int(conf, CONF_ssh_subsys, 0);   /* FIXME: load this properly */
    conf_set_str(conf, CONF_remote_cmd, "");
    conf_set_str(conf, CONF_remote_cmd2, "");
    conf_set_str(conf, CONF_ssh_nc_host, "");

    gpps(sesskey, "HostName", "", conf, CONF_host);
    gppfile(sesskey, "LogFileName", conf, CONF_logfilename);
    gppi(sesskey, "LogType", 0, conf, CONF_logtype);
    gppi(sesskey, "LogFileClash", LGXF_ASK, conf, CONF_logxfovr);
    gppi(sesskey, "LogFlush", 1, conf, CONF_logflush);
    gppi(sesskey, "SSHLogOmitPasswords", 1, conf, CONF_logomitpass);
    gppi(sesskey, "SSHLogOmitData", 0, conf, CONF_logomitdata);

    prot = gpps_raw(sesskey, "Protocol", "default");
    conf_set_int(conf, CONF_protocol, default_protocol);
    conf_set_int(conf, CONF_port, default_port);
    {
	const Backend *b = backend_from_name(prot);
	if (b) {
	    conf_set_int(conf, CONF_protocol, b->protocol);
	    gppi(sesskey, "PortNumber", default_port, conf, CONF_port);
	}
    }
    sfree(prot);

    /* Address family selection */
    gppi(sesskey, "AddressFamily", ADDRTYPE_UNSPEC, conf, CONF_addressfamily);

    /* The CloseOnExit numbers are arranged in a different order from
     * the standard FORCE_ON / FORCE_OFF / AUTO. */
    i = gppi_raw(sesskey, "CloseOnExit", 1); conf_set_int(conf, CONF_close_on_exit, (i+1)%3);
    gppi(sesskey, "WarnOnClose", 1, conf, CONF_warn_on_close);
    {
	/* This is two values for backward compatibility with 0.50/0.51 */
	int pingmin, pingsec;
	pingmin = gppi_raw(sesskey, "PingInterval", 0);
	pingsec = gppi_raw(sesskey, "PingIntervalSecs", 0);
	conf_set_int(conf, CONF_ping_interval, pingmin * 60 + pingsec);
    }
    gppi(sesskey, "TCPNoDelay", 1, conf, CONF_tcp_nodelay);
    gppi(sesskey, "TCPKeepalives", 0, conf, CONF_tcp_keepalives);
    gpps(sesskey, "TerminalType", "xterm", conf, CONF_termtype);
    gpps(sesskey, "TerminalSpeed", "38400,38400", conf, CONF_termspeed);
    if (!gppmap(sesskey, "TerminalModes", conf, CONF_ttymodes)) {
	/* This hardcodes a big set of defaults in any new saved
	 * sessions. Let's hope we don't change our mind. */
	for (i = 0; ttymodes[i]; i++)
	    conf_set_str_str(conf, CONF_ttymodes, ttymodes[i], "A");
    }

    /* proxy settings */
    gpps(sesskey, "ProxyExcludeList", "", conf, CONF_proxy_exclude_list);
    i = gppi_raw(sesskey, "ProxyDNS", 1); conf_set_int(conf, CONF_proxy_dns, (i+1)%3);
    gppi(sesskey, "ProxyLocalhost", 0, conf, CONF_even_proxy_localhost);
    gppi(sesskey, "ProxyMethod", -1, conf, CONF_proxy_type);
    if (conf_get_int(conf, CONF_proxy_type) == -1) {
        int i;
        i = gppi_raw(sesskey, "ProxyType", 0);
        if (i == 0)
            conf_set_int(conf, CONF_proxy_type, PROXY_NONE);
        else if (i == 1)
            conf_set_int(conf, CONF_proxy_type, PROXY_HTTP);
        else if (i == 3)
            conf_set_int(conf, CONF_proxy_type, PROXY_TELNET);
        else if (i == 4)
            conf_set_int(conf, CONF_proxy_type, PROXY_CMD);
        else {
            i = gppi_raw(sesskey, "ProxySOCKSVersion", 5);
            if (i == 5)
                conf_set_int(conf, CONF_proxy_type, PROXY_SOCKS5);
            else
                conf_set_int(conf, CONF_proxy_type, PROXY_SOCKS4);
        }
    }
    gpps(sesskey, "ProxyHost", "proxy", conf, CONF_proxy_host);
    gppi(sesskey, "ProxyPort", 80, conf, CONF_proxy_port);
    gpps(sesskey, "ProxyUsername", "", conf, CONF_proxy_username);
    gpps(sesskey, "ProxyPassword", "", conf, CONF_proxy_password);
    gpps(sesskey, "ProxyTelnetCommand", "connect %host %port\\n",
	 conf, CONF_proxy_telnet_command);
    gppmap(sesskey, "Environment", conf, CONF_environmt);
    gpps(sesskey, "UserName", "", conf, CONF_username);
    gppi(sesskey, "UserNameFromEnvironment", 0, conf, CONF_username_from_env);
    gpps(sesskey, "LocalUserName", "", conf, CONF_localusername);
    gppi(sesskey, "NoPTY", 0, conf, CONF_nopty);
    gppi(sesskey, "Compression", 0, conf, CONF_compression);
    gppi(sesskey, "TryAgent", 1, conf, CONF_tryagent);
    gppi(sesskey, "AgentFwd", 0, conf, CONF_agentfwd);
    gppi(sesskey, "ChangeUsername", 0, conf, CONF_change_username);
    gppi(sesskey, "GssapiFwd", 0, conf, CONF_gssapifwd);
    gprefs(sesskey, "Cipher", "\0",
	   ciphernames, CIPHER_MAX, conf, CONF_ssh_cipherlist);
    {
	/* Backward-compatibility: we used to have an option to
	 * disable gex under the "bugs" panel after one report of
	 * a server which offered it then choked, but we never got
	 * a server version string or any other reports. */
	char *default_kexes;
	i = 2 - gppi_raw(sesskey, "BugDHGEx2", 0);
	if (i == FORCE_ON)
	    default_kexes = "dh-group14-sha1,dh-group1-sha1,rsa,WARN,dh-gex-sha1";
	else
	    default_kexes = "dh-gex-sha1,dh-group14-sha1,dh-group1-sha1,rsa,WARN";
	gprefs(sesskey, "KEX", default_kexes,
	       kexnames, KEX_MAX, conf, CONF_ssh_kexlist);
    }
    gppi(sesskey, "RekeyTime", 60, conf, CONF_ssh_rekey_time);
    gpps(sesskey, "RekeyBytes", "1G", conf, CONF_ssh_rekey_data);
    /* SSH-2 only by default */
    gppi(sesskey, "SshProt", 3, conf, CONF_sshprot);
    gpps(sesskey, "LogHost", "", conf, CONF_loghost);
    gppi(sesskey, "SSH2DES", 0, conf, CONF_ssh2_des_cbc);
    gppi(sesskey, "SshNoAuth", 0, conf, CONF_ssh_no_userauth);
    gppi(sesskey, "SshBanner", 1, conf, CONF_ssh_show_banner);
    gppi(sesskey, "AuthTIS", 0, conf, CONF_try_tis_auth);
    gppi(sesskey, "AuthKI", 1, conf, CONF_try_ki_auth);
    gppi(sesskey, "AuthGSSAPI", 1, conf, CONF_try_gssapi_auth);
#ifndef NO_GSSAPI
    gprefs(sesskey, "GSSLibs", "\0",
	   gsslibkeywords, ngsslibs, conf, CONF_ssh_gsslist);
    gppfile(sesskey, "GSSCustom", conf, CONF_ssh_gss_custom);
#endif
    gppi(sesskey, "SshNoShell", 0, conf, CONF_ssh_no_shell);
    gppfile(sesskey, "PublicKeyFile", conf, CONF_keyfile);
    gpps(sesskey, "RemoteCommand", "", conf, CONF_remote_cmd);
    gppi(sesskey, "RFCEnviron", 0, conf, CONF_rfc_environ);
    gppi(sesskey, "PassiveTelnet", 0, conf, CONF_passive_telnet);
    gppi(sesskey, "BackspaceIsDelete", 1, conf, CONF_bksp_is_delete);
    gppi(sesskey, "RXVTHomeEnd", 0, conf, CONF_rxvt_homeend);
    gppi(sesskey, "LinuxFunctionKeys", 0, conf, CONF_funky_type);
    gppi(sesskey, "NoApplicationKeys", 0, conf, CONF_no_applic_k);
    gppi(sesskey, "NoApplicationCursors", 0, conf, CONF_no_applic_c);
    gppi(sesskey, "NoMouseReporting", 0, conf, CONF_no_mouse_rep);
    gppi(sesskey, "NoRemoteResize", 0, conf, CONF_no_remote_resize);
    gppi(sesskey, "NoAltScreen", 0, conf, CONF_no_alt_screen);
    gppi(sesskey, "NoRemoteWinTitle", 0, conf, CONF_no_remote_wintitle);
    {
	/* Backward compatibility */
	int no_remote_qtitle = gppi_raw(sesskey, "NoRemoteQTitle", 1);
	/* We deliberately interpret the old setting of "no response" as
	 * "empty string". This changes the behaviour, but hopefully for
	 * the better; the user can always recover the old behaviour. */
	gppi(sesskey, "RemoteQTitleAction",
	     no_remote_qtitle ? TITLE_EMPTY : TITLE_REAL,
	     conf, CONF_remote_qtitle_action);
    }
    gppi(sesskey, "NoDBackspace", 0, conf, CONF_no_dbackspace);
    gppi(sesskey, "NoRemoteCharset", 0, conf, CONF_no_remote_charset);
    gppi(sesskey, "ApplicationCursorKeys", 0, conf, CONF_app_cursor);
    gppi(sesskey, "ApplicationKeypad", 0, conf, CONF_app_keypad);
    gppi(sesskey, "NetHackKeypad", 0, conf, CONF_nethack_keypad);
    gppi(sesskey, "AltF4", 1, conf, CONF_alt_f4);
    gppi(sesskey, "AltSpace", 0, conf, CONF_alt_space);
    gppi(sesskey, "AltOnly", 0, conf, CONF_alt_only);
    gppi(sesskey, "ComposeKey", 0, conf, CONF_compose_key);
    gppi(sesskey, "CtrlAltKeys", 1, conf, CONF_ctrlaltkeys);
    gppi(sesskey, "TelnetKey", 0, conf, CONF_telnet_keyboard);
    gppi(sesskey, "TelnetRet", 1, conf, CONF_telnet_newline);
    gppi(sesskey, "LocalEcho", AUTO, conf, CONF_localecho);
    gppi(sesskey, "LocalEdit", AUTO, conf, CONF_localedit);
    gpps(sesskey, "Answerback", "PuTTY", conf, CONF_answerback);
    gppi(sesskey, "AlwaysOnTop", 0, conf, CONF_alwaysontop);
    gppi(sesskey, "FullScreenOnAltEnter", 0, conf, CONF_fullscreenonaltenter);
    gppi(sesskey, "HideMousePtr", 0, conf, CONF_hide_mouseptr);
    gppi(sesskey, "SunkenEdge", 0, conf, CONF_sunken_edge);
    gppi(sesskey, "WindowBorder", 1, conf, CONF_window_border);
    gppi(sesskey, "CurType", 0, conf, CONF_cursor_type);
    gppi(sesskey, "BlinkCur", 0, conf, CONF_blink_cur);
    /* pedantic compiler tells me I can't use conf, CONF_beep as an int * :-) */
    gppi(sesskey, "Beep", 1, conf, CONF_beep);
    gppi(sesskey, "BeepInd", 0, conf, CONF_beep_ind);
    gppfile(sesskey, "BellWaveFile", conf, CONF_bell_wavefile);
    gppi(sesskey, "BellOverload", 1, conf, CONF_bellovl);
    gppi(sesskey, "BellOverloadN", 5, conf, CONF_bellovl_n);
    i = gppi_raw(sesskey, "BellOverloadT", 2*TICKSPERSEC
#ifdef PUTTY_UNIX_H
				   *1000
#endif
				   );
    conf_set_int(conf, CONF_bellovl_t, i
#ifdef PUTTY_UNIX_H
		 / 1000
#endif
		 );
    i = gppi_raw(sesskey, "BellOverloadS", 5*TICKSPERSEC
#ifdef PUTTY_UNIX_H
				   *1000
#endif
				   );
    conf_set_int(conf, CONF_bellovl_s, i
#ifdef PUTTY_UNIX_H
		 / 1000
#endif
		 );
    gppi(sesskey, "ScrollbackLines", 2000, conf, CONF_savelines);
    gppi(sesskey, "DECOriginMode", 0, conf, CONF_dec_om);
    gppi(sesskey, "AutoWrapMode", 1, conf, CONF_wrap_mode);
    gppi(sesskey, "LFImpliesCR", 0, conf, CONF_lfhascr);
    gppi(sesskey, "CRImpliesLF", 0, conf, CONF_crhaslf);
    gppi(sesskey, "DisableArabicShaping", 0, conf, CONF_arabicshaping);
    gppi(sesskey, "DisableBidi", 0, conf, CONF_bidi);
    gppi(sesskey, "WinNameAlways", 1, conf, CONF_win_name_always);
    gpps(sesskey, "WinTitle", "", conf, CONF_wintitle);
    gppi(sesskey, "TermWidth", 80, conf, CONF_width);
    gppi(sesskey, "TermHeight", 24, conf, CONF_height);
    gppfont(sesskey, "Font", conf, CONF_font);
    gppi(sesskey, "FontQuality", FQ_DEFAULT, conf, CONF_font_quality);
    gppi(sesskey, "FontVTMode", VT_UNICODE, conf, CONF_vtmode);
    gppi(sesskey, "UseSystemColours", 0, conf, CONF_system_colour);
    gppi(sesskey, "TryPalette", 0, conf, CONF_try_palette);
    gppi(sesskey, "ANSIColour", 1, conf, CONF_ansi_colour);
    gppi(sesskey, "Xterm256Colour", 1, conf, CONF_xterm_256_colour);
    i = gppi_raw(sesskey, "BoldAsColour", 1); conf_set_int(conf, CONF_bold_style, i+1);

    for (i = 0; i < 22; i++) {
	static const char *const defaults[] = {
	    "187,187,187", "255,255,255", "0,0,0", "85,85,85", "0,0,0",
	    "0,255,0", "0,0,0", "85,85,85", "187,0,0", "255,85,85",
	    "0,187,0", "85,255,85", "187,187,0", "255,255,85", "0,0,187",
	    "85,85,255", "187,0,187", "255,85,255", "0,187,187",
	    "85,255,255", "187,187,187", "255,255,255"
	};
	char buf[20], *buf2;
	int c0, c1, c2;
	sprintf(buf, "Colour%d", i);
	buf2 = gpps_raw(sesskey, buf, defaults[i]);
	if (sscanf(buf2, "%d,%d,%d", &c0, &c1, &c2) == 3) {
	    conf_set_int_int(conf, CONF_colours, i*3+0, c0);
	    conf_set_int_int(conf, CONF_colours, i*3+1, c1);
	    conf_set_int_int(conf, CONF_colours, i*3+2, c2);
	}
	sfree(buf2);
    }
    gppi(sesskey, "RawCNP", 0, conf, CONF_rawcnp);
    gppi(sesskey, "PasteRTF", 0, conf, CONF_rtf_paste);
    gppi(sesskey, "MouseIsXterm", 0, conf, CONF_mouse_is_xterm);
    gppi(sesskey, "RectSelect", 0, conf, CONF_rect_select);
    gppi(sesskey, "MouseOverride", 1, conf, CONF_mouse_override);
    for (i = 0; i < 256; i += 32) {
	static const char *const defaults[] = {
	    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
	    "0,1,2,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1",
	    "1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,2",
	    "1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1",
	    "1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
	    "1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
	    "2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2",
	    "2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2"
	};
	char buf[20], *buf2, *p;
	int j;
	sprintf(buf, "Wordness%d", i);
	buf2 = gpps_raw(sesskey, buf, defaults[i / 32]);
	p = buf2;
	for (j = i; j < i + 32; j++) {
	    char *q = p;
	    while (*p && *p != ',')
		p++;
	    if (*p == ',')
		*p++ = '\0';
	    conf_set_int_int(conf, CONF_wordness, j, atoi(q));
	}
	sfree(buf2);
    }
    /*
     * The empty default for LineCodePage will be converted later
     * into a plausible default for the locale.
     */
    gpps(sesskey, "LineCodePage", "", conf, CONF_line_codepage);
    gppi(sesskey, "CJKAmbigWide", 0, conf, CONF_cjk_ambig_wide);
    gppi(sesskey, "UTF8Override", 1, conf, CONF_utf8_override);
    gpps(sesskey, "Printer", "", conf, CONF_printer);
    gppi(sesskey, "CapsLockCyr", 0, conf, CONF_xlat_capslockcyr);
    gppi(sesskey, "ScrollBar", 1, conf, CONF_scrollbar);
    gppi(sesskey, "ScrollBarFullScreen", 0, conf, CONF_scrollbar_in_fullscreen);
    gppi(sesskey, "ScrollOnKey", 0, conf, CONF_scroll_on_key);
    gppi(sesskey, "ScrollOnDisp", 1, conf, CONF_scroll_on_disp);
    gppi(sesskey, "EraseToScrollback", 1, conf, CONF_erase_to_scrollback);
    gppi(sesskey, "LockSize", 0, conf, CONF_resize_action);
    gppi(sesskey, "BCE", 1, conf, CONF_bce);
    gppi(sesskey, "BlinkText", 0, conf, CONF_blinktext);
    gppi(sesskey, "X11Forward", 0, conf, CONF_x11_forward);
    gpps(sesskey, "X11Display", "", conf, CONF_x11_display);
    gppi(sesskey, "X11AuthType", X11_MIT, conf, CONF_x11_auth);
    gppfile(sesskey, "X11AuthFile", conf, CONF_xauthfile);

    gppi(sesskey, "LocalPortAcceptAll", 0, conf, CONF_lport_acceptall);
    gppi(sesskey, "RemotePortAcceptAll", 0, conf, CONF_rport_acceptall);
    gppmap(sesskey, "PortForwardings", conf, CONF_portfwd);
    i = gppi_raw(sesskey, "BugIgnore1", 0); conf_set_int(conf, CONF_sshbug_ignore1, 2-i);
    i = gppi_raw(sesskey, "BugPlainPW1", 0); conf_set_int(conf, CONF_sshbug_plainpw1, 2-i);
    i = gppi_raw(sesskey, "BugRSA1", 0); conf_set_int(conf, CONF_sshbug_rsa1, 2-i);
    i = gppi_raw(sesskey, "BugIgnore2", 0); conf_set_int(conf, CONF_sshbug_ignore2, 2-i);
    {
	int i;
	i = gppi_raw(sesskey, "BugHMAC2", 0); conf_set_int(conf, CONF_sshbug_hmac2, 2-i);
	if (2-i == AUTO) {
	    i = gppi_raw(sesskey, "BuggyMAC", 0);
	    if (i == 1)
		conf_set_int(conf, CONF_sshbug_hmac2, FORCE_ON);
	}
    }
    i = gppi_raw(sesskey, "BugDeriveKey2", 0); conf_set_int(conf, CONF_sshbug_derivekey2, 2-i);
    i = gppi_raw(sesskey, "BugRSAPad2", 0); conf_set_int(conf, CONF_sshbug_rsapad2, 2-i);
    i = gppi_raw(sesskey, "BugPKSessID2", 0); conf_set_int(conf, CONF_sshbug_pksessid2, 2-i);
    i = gppi_raw(sesskey, "BugRekey2", 0); conf_set_int(conf, CONF_sshbug_rekey2, 2-i);
    i = gppi_raw(sesskey, "BugMaxPkt2", 0); conf_set_int(conf, CONF_sshbug_maxpkt2, 2-i);
    i = gppi_raw(sesskey, "BugOldGex2", 0); conf_set_int(conf, CONF_sshbug_oldgex2, 2-i);
    i = gppi_raw(sesskey, "BugWinadj", 0); conf_set_int(conf, CONF_sshbug_winadj, 2-i);
    i = gppi_raw(sesskey, "BugChanReq", 0); conf_set_int(conf, CONF_sshbug_chanreq, 2-i);
    conf_set_int(conf, CONF_ssh_simple, FALSE);
    gppi(sesskey, "StampUtmp", 1, conf, CONF_stamp_utmp);
    gppi(sesskey, "LoginShell", 1, conf, CONF_login_shell);
    gppi(sesskey, "ScrollbarOnLeft", 0, conf, CONF_scrollbar_on_left);
    gppi(sesskey, "ShadowBold", 0, conf, CONF_shadowbold);
    gppfont(sesskey, "BoldFont", conf, CONF_boldfont);
    gppfont(sesskey, "WideFont", conf, CONF_widefont);
    gppfont(sesskey, "WideBoldFont", conf, CONF_wideboldfont);
    gppi(sesskey, "ShadowBoldOffset", 1, conf, CONF_shadowboldoffset);
    gpps(sesskey, "SerialLine", "", conf, CONF_serline);
    gppi(sesskey, "SerialSpeed", 9600, conf, CONF_serspeed);
    gppi(sesskey, "SerialDataBits", 8, conf, CONF_serdatabits);
    gppi(sesskey, "SerialStopHalfbits", 2, conf, CONF_serstopbits);
    gppi(sesskey, "SerialParity", SER_PAR_NONE, conf, CONF_serparity);
    gppi(sesskey, "SerialFlowControl", SER_FLOW_XONXOFF, conf, CONF_serflow);
    gpps(sesskey, "WindowClass", "", conf, CONF_winclass);
    gppi(sesskey, "ConnectionSharing", 0, conf, CONF_ssh_connection_sharing);
    gppi(sesskey, "ConnectionSharingUpstream", 1, conf, CONF_ssh_connection_sharing_upstream);
    gppi(sesskey, "ConnectionSharingDownstream", 1, conf, CONF_ssh_connection_sharing_downstream);
    gppmap(sesskey, "SSHManualHostKeys", conf, CONF_ssh_manual_hostkeys);
}

void do_defaults(char *session, Conf *conf)
{
    load_settings(session, conf);
}

static int sessioncmp(const void *av, const void *bv)
{
    const char *a = *(const char *const *) av;
    const char *b = *(const char *const *) bv;

    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(a, "Default Settings"))
	return -1;		       /* a comes first */
    if (!strcmp(b, "Default Settings"))
	return +1;		       /* b comes first */
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return strcmp(a, b);	       /* otherwise, compare normally */
}

void get_sesslist(struct sesslist *list, int allocate)
{
    char otherbuf[2048];
    int buflen, bufsize, i;
    char *p, *ret;
    void *handle;

    if (allocate) {

	buflen = bufsize = 0;
	list->buffer = NULL;
	if ((handle = enum_settings_start()) != NULL) {
	    do {
		ret = enum_settings_next(handle, otherbuf, sizeof(otherbuf));
		if (ret) {
		    int len = strlen(otherbuf) + 1;
		    if (bufsize < buflen + len) {
			bufsize = buflen + len + 2048;
			list->buffer = sresize(list->buffer, bufsize, char);
		    }
		    strcpy(list->buffer + buflen, otherbuf);
		    buflen += strlen(list->buffer + buflen) + 1;
		}
	    } while (ret);
	    enum_settings_finish(handle);
	}
	list->buffer = sresize(list->buffer, buflen + 1, char);
	list->buffer[buflen] = '\0';

	/*
	 * Now set up the list of sessions. Note that "Default
	 * Settings" must always be claimed to exist, even if it
	 * doesn't really.
	 */

	p = list->buffer;
	list->nsessions = 1;	       /* "Default Settings" counts as one */
	while (*p) {
	    if (strcmp(p, "Default Settings"))
		list->nsessions++;
	    while (*p)
		p++;
	    p++;
	}

	list->sessions = snewn(list->nsessions + 1, char *);
	list->sessions[0] = "Default Settings";
	p = list->buffer;
	i = 1;
	while (*p) {
	    if (strcmp(p, "Default Settings"))
		list->sessions[i++] = p;
	    while (*p)
		p++;
	    p++;
	}

	qsort(list->sessions, i, sizeof(char *), sessioncmp);
    } else {
	sfree(list->buffer);
	sfree(list->sessions);
	list->buffer = NULL;
	list->sessions = NULL;
    }
}
