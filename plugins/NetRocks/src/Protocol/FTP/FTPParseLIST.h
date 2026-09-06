#ifndef FTPPARSE_H
#define FTPPARSE_H


/*
ftpparse(&fp,buf,len) tries to parse one line of LIST output.

The line is an array of len characters stored in buf.
It should not include the terminating CR LF; so buf[len] is typically CR.

If ftpparse() can't find a filename, it returns 0.

If ftpparse() can find a filename, it fills in fp and returns 1.
fp is a struct ftpparse, defined below.
The name is an array of fp.namelen characters stored in fp.name;
fp.name points somewhere within buf.
*/

struct ftpparse {
	const char *name; /* not necessarily 0-terminated */
	int namelen;
	int flagtrycwd; /* 0 if cwd is definitely pointless, 1 otherwise */
	int flagtryretr; /* 0 if retr is definitely pointless, 1 otherwise */
	int sizetype;
	long size; /* number of octets */
	int mtimetype;
	time_t mtime; /* modification time */
	int idtype;
	const char *id; /* not necessarily 0-terminated */
	int idlen;

	mode_t mode;
	char owner[0x100];
	char group[0x100];
} ;

#define FTPPARSE_SIZE_UNKNOWN 0
#define FTPPARSE_SIZE_BINARY 1 /* size is the number of octets in TYPE I */
#define FTPPARSE_SIZE_ASCII 2 /* size is the number of octets in TYPE A */

#define FTPPARSE_MTIME_UNKNOWN 0
#define FTPPARSE_MTIME_LOCAL 1 /* time is correct */
#define FTPPARSE_MTIME_REMOTEMINUTE 2 /* time zone and secs are unknown */
#define FTPPARSE_MTIME_REMOTEDAY 3 /* time zone and time of day are unknown */
/*
When a time zone is unknown, it is assumed to be GMT. You may want
to use localtime() for LOCAL times, along with an indication that the
time is correct in the local time zone, and gmtime() for REMOTE* times.
*/

#define FTPPARSE_ID_UNKNOWN 0
#define FTPPARSE_ID_FULL 1 /* unique identifier for files on this FTP server */

int ftpparse(struct ftpparse *, const char *, int);


#endif
