#ifndef FAR_SETTINGS_H
#define FAR_SETTINGS_H
#include <string>
#include <stdint.h>
#include "lng.h"

class Settings
{
    std::string _ini_path;

    bool _open_by_enter = false;
	bool _open_by_cpgdn = true;
    bool _open_in_qv = false;
    bool _open_in_fv = false;

    std::string _image_masks =
				"*.ai *.ani *.avif *.bmp *.bw *.cdr *.cel "
				"*.cgm *.cmx *.cpt *.cur *.dcx *.dds *.dib "
				"*.emf *.eps *.flc *.fli *.fpx *.gif *.icl *.ico "
				"*.iff *.indd *.j2k *.jp2 *.jpc *.jpe *.jpeg "
				"*.jpeg2000 *.jpg *.jps *.kra *.lbm *.mng *.mpo "
				"*.pbm *.pcx *.pdn *.pgm *.pic *.png *.pns "
				"*.ppm *.psd *.psp *.ras *.rgb *.rle *.sai "
				"*.sgi *.spr *.svg *.tga *.tif *.tiff *.wbmp "
				"*.webp *.wmf *.xbm *.xcf *.xpm";

    std::string _video_masks =
				"*.3g2 *.3gp *.asf *.avchd *.avi "
				"*.divx *.enc *.flv *.ifo *.m1v *.m2ts "
				"*.m2v *.m4p *.m4v *.mkv *.mov *.mp2 "
				"*.mp4 *.mpe *.mpeg *.mpg *.mpv *.mts "
				"*.ogm *.qt *.ra *.ram *.rmvb *.swf "
				"*.ts *.vob *.vob *.webm *.wm *.wmv";

public:
	Settings();
    const wchar_t *Msg(int msgId);

    void configurationMenuDialog();

	bool OpenByEnter() const { return _open_by_enter; }
	bool OpenByCtrlPgDn() const { return _open_by_cpgdn; }
	bool OpenInQV() const { return _open_in_qv; }
	bool OpenInFV() const { return _open_in_fv; }

	bool MatchImageFile(const char *name) const;
	bool MatchVideoFile(const char *name) const;
	bool MatchFile(const char *name) const { return MatchImageFile(name) || MatchVideoFile(name); } 
};

extern Settings g_settings;

#endif // FAR_SETTINGS_H
