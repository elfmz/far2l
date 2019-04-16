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
	XDDA_ASK,
	XDDA_SKIP,
	XDDA_RESUME,
	XDDA_OVERWRITE,
	XDOA_OVERWRITE_IF_NEWER,
	XDOA_CREATE_DIFFERENT_NAME
};

