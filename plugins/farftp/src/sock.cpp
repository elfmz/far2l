#include <all_far.h>

#include "Int.h"

void SetSocketBlockingEnabled(int fd, bool blocking)
{
	if (fd < 0)
		return;

#ifdef WIN32
	unsigned long mode = blocking ? 0 : 1;
	ioctlsocket(fd, FIONBIO, &mode);
#else
	if (blocking)
		MakeFDBlocking(fd);
	else
		MakeFDNonBlocking(fd);
#endif
}

void WINAPI scClose(SOCKET &sock, int how)
{
	DWORD err = WINPORT(GetLastError)();

	if (sock != INVALID_SOCKET) {
		Log(("SOCK: closed %d as %d", sock, how));

		if (how != -1)
			shutdown(sock, how);

		int rc = close(sock);
		Log(("SOCK: closesocket()=%d (%s)", rc, GetSocketErrorSTR()));

		if (rc && IS_SOCKET_NONBLOCKING_ERR(errno)) {
			linger l;
			l.l_onoff = 0;
			l.l_linger = 0;

			if ((rc = setsockopt(sock, SOL_SOCKET, SO_LINGER, (char FAR *)&l, sizeof(l))) != 0) {
				Log(("SOCK: Error set linger: %d,%s", rc, GetSocketErrorSTR()));
			} else {
				rc = close(sock);
				Log(("SOCK: 2 closesocket()=%d (%s)", rc, GetSocketErrorSTR()));
			}
		}

		sock = INVALID_SOCKET;
	}

	WINPORT(SetLastError)(err);
	// WINPORT(WSASetLastError)(serr);
}

BOOL WINAPI scValid(SOCKET sock)
{
	return sock != INVALID_SOCKET;
}

SOCKET WINAPI scCreate(short addr_type)
{
	SOCKET s;
	int iv;
	// u_long ul;
	s = socket(addr_type, SOCK_STREAM, IPPROTO_TCP);

	if (!scValid(s))
		return INVALID_SOCKET;

	Log(("SOCK: created %d", s));

	do {
		iv = TRUE;

		//		if(setsockopt(s,SOL_SOCKET,SO_DONTLINGER,(char FAR *)&iv,sizeof(iv)) != 0) break;

		//		iv = TRUE;

		//		if(setsockopt(s,SOL_SOCKET,SO_DONTROUTE,(char FAR *)&iv,sizeof(iv)) != 0) break;

		iv = TRUE;

		if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char FAR *)&iv, sizeof(iv)) != 0)
			break;

#ifdef SO_OOBINLINE
		iv = 1;

		if (setsockopt(s, SOL_SOCKET, SO_OOBINLINE, (char FAR *)&iv, sizeof(iv)) != 0)
			break;

#endif
		SetSocketBlockingEnabled(s, false);

		return s;
	} while (0);

	scClose(s);
	return INVALID_SOCKET;
}

SOCKET WINAPI scAccept(SOCKET *peer, struct sockaddr FAR *addr, socklen_t *addrlen)
{
	SOCKET s = accept(*peer, addr, addrlen);

	if (!scValid(s))
		return INVALID_SOCKET;

	Log(("SOCK: accepted %d at %d", s, *peer));
	return s;
}
