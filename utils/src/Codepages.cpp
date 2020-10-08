#include <Codepages.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace Codepages
{
	#define IsLocaleMatches(current, wanted_literal) \
		( strncmp((current), wanted_literal, sizeof(wanted_literal) - 1) == 0 && \
		( (current)[sizeof(wanted_literal) - 1] == 0 || (current)[sizeof(wanted_literal) - 1] == '.') )

	unsigned int DetectOemCP()
	{
		// detect oem cp from system locale
		int oem_cp = 866; // default

		const char *lc = setlocale(LC_CTYPE, NULL);
		if (!lc) { fprintf(stderr, "DetectOemCP: setlocale returned NULL\n"); }
		else if (IsLocaleMatches(lc, "af_ZA")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ar_SA")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_LB")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_EG")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_DZ")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_BH")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_IQ")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_JO")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_KW")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_LY")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_MA")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_OM")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_QA")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_SY")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_TN")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_AE")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ar_YE")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "ast_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "az_AZ")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "az_AZ")) { oem_cp = 857; }
		else if (IsLocaleMatches(lc, "be_BY")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "bg_BG")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "br_FR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ca_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "zh_CN")) { oem_cp = 936; }
		else if (IsLocaleMatches(lc, "zh_TW")) { oem_cp = 950; }
		else if (IsLocaleMatches(lc, "kw_GB")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "cs_CZ")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "cy_GB")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "da_DK")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_AT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_LI")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_LU")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_CH")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "de_DE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "el_GR")) { oem_cp = 737; }
		else if (IsLocaleMatches(lc, "en_AU")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_CA")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_GB")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_IE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_JM")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_BZ")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_PH")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "en_ZA")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "en_TT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "en_US")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "en_ZW")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "en_NZ")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_PA")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_BO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_CR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_DO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_SV")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_EC")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_GT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_HN")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_NI")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_CL")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_MX")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_CO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_PE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_AR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_PR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_VE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_UY")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "es_PY")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "et_EE")) { oem_cp = 775; }
		else if (IsLocaleMatches(lc, "eu_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fa_IR")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "fi_FI")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fo_FO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_FR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_BE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_CA")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_LU")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_MC")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "fr_CH")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ga_IE")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "gd_GB")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "gv_IM")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "gl_ES")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "he_IL")) { oem_cp = 862; }
		else if (IsLocaleMatches(lc, "hr_HR")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "hu_HU")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "id_ID")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "is_IS")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "it_IT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "it_CH")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "iv_IV")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "ja_JP")) { oem_cp = 932; }
		else if (IsLocaleMatches(lc, "kk_KZ")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "ko_KR")) { oem_cp = 949; }
		else if (IsLocaleMatches(lc, "ky_KG")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "lt_LT")) { oem_cp = 775; }
		else if (IsLocaleMatches(lc, "lv_LV")) { oem_cp = 775; }
		else if (IsLocaleMatches(lc, "mk_MK")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "mn_MN")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "ms_BN")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ms_MY")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nl_BE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nl_NL")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nl_SR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nn_NO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "nb_NO")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "pl_PL")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "pt_BR")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "pt_PT")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "rm_CH")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "ro_RO")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "ru_RU")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "sk_SK")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "sl_SI")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "sq_AL")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "sr_RS")) { oem_cp = 855; }
		else if (IsLocaleMatches(lc, "sr_RS")) { oem_cp = 852; }
		else if (IsLocaleMatches(lc, "sv_SE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "sv_FI")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "sw_KE")) { oem_cp = 437; }
		else if (IsLocaleMatches(lc, "th_TH")) { oem_cp = 874; }
		else if (IsLocaleMatches(lc, "tr_TR")) { oem_cp = 857; }
		else if (IsLocaleMatches(lc, "tt_RU")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "uk_UA")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "ur_PK")) { oem_cp = 720; }
		else if (IsLocaleMatches(lc, "uz_UZ")) { oem_cp = 866; }
		else if (IsLocaleMatches(lc, "uz_UZ")) { oem_cp = 857; }
		else if (IsLocaleMatches(lc, "vi_VN")) { oem_cp = 1258; }
		else if (IsLocaleMatches(lc, "wa_BE")) { oem_cp = 850; }
		else if (IsLocaleMatches(lc, "zh_HK")) { oem_cp = 950; }
		else if (IsLocaleMatches(lc, "zh_SG")) { oem_cp = 936; }
		else if (IsLocaleMatches(lc, "zh_MO")) { oem_cp = 950; }
		else {
			fprintf(stderr, "DetectOemCP: setlocale returned unexpected '%s'\n", lc);
		}

//		fprintf(stderr, "DetectOemCP: returning %u\n", oem_cp);

		return oem_cp;
	}
}
