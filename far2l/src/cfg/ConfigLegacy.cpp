#include "headers.hpp"
#include "ConfigRW.hpp"
#include <errno.h>

#ifdef WINPORT_REGISTRY
static bool ShouldImportRegSettings(const std::string &virtual_path)
{
	return (
		// dont care about legacy Plugins settings
		virtual_path != "Plugins"

		// skip stuff that goes to plugins/state.ini
		&& virtual_path != "PluginHotkeys"
		&& virtual_path != "PluginsCache"

		// skip abandoned poscache entries
		&& virtual_path != "Viewer/LastPositions"
		&& virtual_path != "Editor/LastPositions"

		// separately handled by BookmarksLegacy.cpp
		&& virtual_path != "FolderShortcuts"
		);
}

static void ConfigUgrade_RegKey(FILE *lf, ConfigWriter &cfg_writer, HKEY root, const wchar_t *subpath, const std::string &virtual_path)
{
	HKEY key = 0;
	LONG r = WINPORT(RegOpenKeyEx)(root, subpath, 0, GENERIC_READ, &key);
	std::string virtual_subpath;
	if (r == ERROR_SUCCESS) {
		std::vector<wchar_t> namebuf(0x400);
		for (DWORD i = 0; ;++i) {
			r = WINPORT(RegEnumKey)(key, i, &namebuf[0], namebuf.size() - 1);
			if (r != ERROR_SUCCESS) break;
			virtual_subpath = virtual_path;
			if (!virtual_subpath.empty()) {
				virtual_subpath+= '/';
			}
			virtual_subpath+= Wide2MB(&namebuf[0]);
			if (ShouldImportRegSettings(virtual_subpath)) {
				fprintf(lf, "%s: RECURSE '%s'\n", __FUNCTION__, virtual_subpath.c_str());
				ConfigUgrade_RegKey(lf, cfg_writer, key, &namebuf[0], virtual_subpath);
			} else {
				fprintf(lf, "%s: SKIP '%s'\n", __FUNCTION__, virtual_subpath.c_str());
			}
		}

		fprintf(lf, "%s: ENUM '%s'\n", __FUNCTION__, virtual_path.c_str());
		cfg_writer.SelectSection(virtual_path);
		const bool macro_type_prefix =
			(virtual_path == "KeyMacros/Vars" || virtual_path == "KeyMacros/Consts");

		std::vector<BYTE> databuf(0x400);
		for (DWORD i = 0; ;) {
			DWORD namelen = namebuf.size() - 1;
			DWORD datalen = databuf.size();
			DWORD tip = 0;
			r = WINPORT(RegEnumValue)(key, i,
				&namebuf[0], &namelen, NULL, &tip, &databuf[0], &datalen);
			if (r != ERROR_SUCCESS) {
				if (r != ERROR_MORE_DATA) {
					break;
				}
				namebuf.resize(namebuf.size() + 0x400);
				databuf.resize(databuf.size() + 0x400);

			} else {
				fprintf(lf, "%s: SET [%s] '%ls' TYPE=%u DATA={",
					__FUNCTION__, virtual_path.c_str(), &namebuf[0], tip);
				for (size_t j = 0; j < datalen; ++j) {
					fprintf(lf, "%02x ", (unsigned int)databuf[j]);
				}
				fprintf(lf, "}\n");

				std::string name(Wide2MB(&namebuf[0]));
				FARString tmp_str;
				switch (tip) {
					case REG_DWORD: {
						if (macro_type_prefix) {
							tmp_str.Format(L"INT:%ld", (long)*(int32_t *)&databuf[0]);
							cfg_writer.SetString(name, tmp_str);
						} else {
							cfg_writer.SetUInt(name, (unsigned int)*(uint32_t *)&databuf[0]);
						}
					} break;
					case REG_QWORD: {
						if (macro_type_prefix) {
							tmp_str.Format(L"INT:%lld", (long long)*(int64_t *)&databuf[0]);
							cfg_writer.SetString(name, tmp_str);
						} else {
							cfg_writer.SetULL(name, *(uint64_t *)&databuf[0]);
						}
					} break;
					case REG_SZ: case REG_EXPAND_SZ: case REG_MULTI_SZ: {
							if (macro_type_prefix) {
								tmp_str = L"STR:";
							}
							for (size_t i = 0; i + sizeof(WCHAR) <= datalen ; i+= sizeof(WCHAR)) {
								WCHAR wc = *(const WCHAR *)&databuf[i];
								if (!wc) {
									if (i + sizeof(WCHAR) >= datalen) {
										break;
									}
									// REG_MULTI_SZ was used only in macroses and history.
									// Macroses did inter-strings zeroes translated to \n.
									// Now macroses code doesn't do that translation, just need
									// to one time translate data imported from legacy registry.
									// History code now also uses '\n' chars as string separators.
									if (tip == REG_MULTI_SZ) {
										if (i + 2 * sizeof(WCHAR) >= datalen) {
											// skip last string terminator translation
											break;
										}
										tmp_str+= L'\n';
									} else {
										tmp_str.Append(wc);
									}
								} else {
									tmp_str.Append(wc);
								}
							}
							cfg_writer.SetString(name, tmp_str);
					} break;

					default:
						cfg_writer.SetBytes(name, &databuf[0], datalen);
				}
				++i;
			}
		}
		WINPORT(RegCloseKey)(key);
	}
}

static void Upgrade_MoveFile(FILE *lf, const char *src, const char *dst)
{
	const std::string &str_src = InMyConfig(src);
	const std::string &str_dst = InMyConfig(dst);
	int r = rename(str_src.c_str(), str_dst.c_str());
	if (r == 0) {
		fprintf(lf, "%s('%s', '%s') - DONE\n", __FUNCTION__, str_src.c_str(), str_dst.c_str());
	} else {
		fprintf(lf, "%s('%s', '%s') - ERROR=%d\n", __FUNCTION__, str_src.c_str(), str_dst.c_str(), errno);
	}
}

void ConfigLegacyUpgrade()
{
	const std::string &cfg_ini = InMyConfig(CONFIG_INI);
	struct stat s{};
	if (stat(cfg_ini.c_str(), &s) == -1) {
		FILE *lf = fopen(InMyCache("upgrade.log").c_str(), "a");
		if (!lf) {
			lf = stderr;
		}
		try {
			const std::string sh_log = InMyCache("upgrade.sh.log");
			int r = system(StrPrintf("date >>\"%s\" 2>&1", sh_log.c_str()).c_str());
			if (r != 0) {
				perror("system(date)");
			}

			time_t now = time(NULL);
			fprintf(lf, "---- Upgrade started on %s\n", ctime(&now));
			ConfigWriter cfg_writer;
			ConfigUgrade_RegKey(lf, cfg_writer, HKEY_CURRENT_USER, L"Software/Far2", "");
			Upgrade_MoveFile(lf, "bookmarks.ini", "settings/bookmarks.ini");
			Upgrade_MoveFile(lf, "plugins.ini", "plugins/state.ini");
			Upgrade_MoveFile(lf, "viewer.pos", "history/viewer.pos");
			Upgrade_MoveFile(lf, "editor.pos", "history/editor.pos");

			const std::string &cmd = StrPrintf(
				"mv -f \"%s\" \"%s\" >>\"%s\" 2>&1 || cp -f -r \"%s\" \"%s\" >>\"%s\" 2>&1",
				InMyConfig("NetRocks").c_str(),
				InMyConfig("plugins/").c_str(),
				sh_log.c_str(),
				InMyConfig("NetRocks").c_str(),
				InMyConfig("plugins/").c_str(),
				sh_log.c_str() );

			r = system(cmd.c_str());
			if (r != 0) {
				fprintf(stderr, "%s: ERROR=%d CMD='%s'\n", __FUNCTION__, r, cmd.c_str());
				fprintf(lf, "%s: ERROR=%d CMD='%s'\n", __FUNCTION__, r, cmd.c_str());
			} else {
				fprintf(lf, "%s: DONE CMD='%s'\n", __FUNCTION__, cmd.c_str());
			}

			now = time(NULL);
			cfg_writer.SelectSection("Upgrade");
			cfg_writer.SetULL("UpgradedOn", (unsigned long long)now);
			fprintf(lf, "---- Upgrade finished on %s\n", ctime(&now));

		} catch (std::exception &e) {
			fprintf(stderr, "%s: EXCEPTION: %s\n", __FUNCTION__, e.what());
			fprintf(lf, "%s: EXCEPTION: %s\n", __FUNCTION__, e.what());
		}
		if (lf != stderr) {
			fclose(lf);
		}
	}
}

#else // WINPORT_REGISTRY

void ConfigLegacyUpgrade() { }

#endif // WINPORT_REGISTRY
