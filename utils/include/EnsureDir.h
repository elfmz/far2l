#pragma once
enum PrivacyLevel
{
	// if dir exists and accessible - don't care who owns it, if not exists - create accessible for all (0777)
	PL_ALL = 0,

	// if dir exists and accessible - don't care who owns it, if not exists - create accessible to current user only
	PL_ANY,

	// fail if existing dir owned by nor current user nor root
	PL_PRIVATE
};

// ensures that specified dir exists and accessible for caller with optional extra privacy restrictions
bool EnsureDir(const char *dir, PrivacyLevel pl = PL_ANY);
