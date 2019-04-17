#pragma once

enum XferKind
{
	XK_COPY,
	XK_MOVE
};

enum XferDirection
{
	XK_DOWNLOAD,
	XK_UPLOAD
};

enum XferDefaultOverwriteAction
{
	XDOA_ASK,
	XDOA_SKIP,
	XDOA_RESUME,
	XDOA_OVERWRITE,
	XDOA_OVERWRITE_IF_NEWER,
	XDOA_CREATE_DIFFERENT_NAME
};

