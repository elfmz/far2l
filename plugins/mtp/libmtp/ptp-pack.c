/* ptp-pack.c
 *
 * Copyright (C) 2001-2004 Mariusz Woloszyn <emsi@ipartners.pl>
 * Copyright (C) 2003-2019 Marcus Meissner <marcus@jet.franken.de>
 * Copyright (C) 2006-2008 Linus Walleij <triad@df.lth.se>
 * Copyright (C) 2007 Tero Saarni <tero.saarni@gmail.com>
 * Copyright (C) 2009 Axel Waggershauser <awagger@web.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/* currently this file is included into ptp.c */

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef UINT_MAX
# define UINT_MAX 0xFFFFFFFF
#endif
#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_H)
#include <iconv.h>
#endif

static inline uint16_t
htod16p (PTPParams *params, uint16_t var)
{
	return ((params->byteorder==PTP_DL_LE)?htole16(var):htobe16(var));
}

static inline uint32_t
htod32p (PTPParams *params, uint32_t var)
{
	return ((params->byteorder==PTP_DL_LE)?htole32(var):htobe32(var));
}

static inline void
htod16ap (PTPParams *params, unsigned char *a, uint16_t val)
{
	if (params->byteorder==PTP_DL_LE)
		htole16a(a,val);
	else
		htobe16a(a,val);
}

static inline void
htod32ap (PTPParams *params, unsigned char *a, uint32_t val)
{
	if (params->byteorder==PTP_DL_LE)
		htole32a(a,val);
	else
		htobe32a(a,val);
}

static inline void
htod64ap (PTPParams *params, unsigned char *a, uint64_t val)
{
	if (params->byteorder==PTP_DL_LE)
		htole64a(a,val);
	else
		htobe64a(a,val);
}

static inline uint16_t
dtoh16p (PTPParams *params, uint16_t var)
{
	return ((params->byteorder==PTP_DL_LE)?le16toh(var):be16toh(var));
}

static inline uint32_t
dtoh32p (PTPParams *params, uint32_t var)
{
	return ((params->byteorder==PTP_DL_LE)?le32toh(var):be32toh(var));
}

static inline uint64_t
dtoh64p (PTPParams *params, uint64_t var)
{
	return ((params->byteorder==PTP_DL_LE)?le64toh(var):be64toh(var));
}

static inline uint16_t
dtoh16ap (PTPParams *params, const unsigned char *a)
{
	return ((params->byteorder==PTP_DL_LE)?le16atoh(a):be16atoh(a));
}

static inline uint32_t
dtoh32ap (PTPParams *params, const unsigned char *a)
{
	return ((params->byteorder==PTP_DL_LE)?le32atoh(a):be32atoh(a));
}

static inline uint64_t
dtoh64ap (PTPParams *params, const unsigned char *a)
{
	return ((params->byteorder==PTP_DL_LE)?le64atoh(a):be64atoh(a));
}

#define htod8a(a,x)	*(uint8_t*)(a) = x
#define htod16a(a,x)	htod16ap(params,a,x)
#define htod32a(a,x)	htod32ap(params,a,x)
#define htod64a(a,x)	htod64ap(params,a,x)
#define htod16(x)	htod16p(params,x)
#define htod32(x)	htod32p(params,x)
#define htod64(x)	htod64p(params,x)

#define dtoh8a(x)	(*(uint8_t*)(x))
#define dtoh16a(a)	dtoh16ap(params,a)
#define dtoh32a(a)	dtoh32ap(params,a)
#define dtoh64a(a)	dtoh64ap(params,a)
#define dtoh16(x)	dtoh16p(params,x)
#define dtoh32(x)	dtoh32p(params,x)
#define dtoh64(x)	dtoh64p(params,x)


/*
 * PTP strings ... if the size field is:
 * size 0  : "empty string" ... we interpret that as string with just \0 terminator, return 1
 *  (the whole PTP standard is not that clear, it occasionaly refers to strings as optional in such cases, but no clear guidance).
 * size > 0: all other strings have a terminating \0, included in the length (not sure how conforming everyone is here)
 *
 * len - in ptp string characters currently
 */
static inline int
ptp_unpack_string(PTPParams *params, unsigned char* data, uint32_t offset, uint32_t total, uint8_t *len, char **retstr)
{
	uint8_t length;
	uint16_t string[PTP_MAXSTRLEN+1];
	/* allow for UTF-8: max of 3 bytes per UCS-2 char, plus final null */
	char loclstr[PTP_MAXSTRLEN*3+1];
	size_t nconv, srclen, destlen;
	char *src, *dest;

	*len = 0;
	*retstr = NULL;

	if (offset + 1 > total)
		return 0;

	length = dtoh8a(&data[offset]);	/* PTP_MAXSTRLEN == 255, 8 bit len */
	if (length == 0) {		/* nothing to do? */
		*len = 0;
		*retstr = strdup("");	/* return an empty string, not NULL */
		return 1;
	}

	if (offset + 1 + length*sizeof(string[0]) > total)
		return 0;

	*len = length;

	/* copy to string[] to ensure correct alignment for iconv(3) */
	memcpy(string, &data[offset+1], length * sizeof(string[0]));
	string[length] = 0x0000U;   /* be paranoid!  add a terminator. */
	loclstr[0] = '\0';

	/* convert from camera UCS-2 to our locale */
	src = (char *)string;
	srclen = length * sizeof(string[0]);
	dest = loclstr;
	destlen = sizeof(loclstr)-1;
	nconv = (size_t)-1;
#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_H)
	if (params->cd_ucs2_to_locale != (iconv_t)-1)
		nconv = iconv(params->cd_ucs2_to_locale, &src, &srclen, &dest, &destlen);
#endif
	if (nconv == (size_t) -1) { /* do it the hard way */
		int i;
		/* try the old way, in case iconv is broken */
		for (i=0;i<length;i++) {
			if (dtoh16a(&data[offset+1+2*i])>127)
				loclstr[i] = '?';
			else
				loclstr[i] = dtoh16a(&data[offset+1+2*i]);
		}
		dest = loclstr+length;
	}
	*dest = '\0';
	loclstr[sizeof(loclstr)-1] = '\0';   /* be safe? */
	*retstr = strdup(loclstr);
	return 1;
}

static inline int
ucs2strlen(uint16_t const * const unicstr)
{
	int length = 0;

	/* Unicode strings are terminated with 2 * 0x00 */
	for(length = 0; unicstr[length] != 0x0000U; length ++);
	return length;
}


static inline void
ptp_pack_string(PTPParams *params, char *string, unsigned char* data, uint16_t offset, uint8_t *len)
{
	int packedlen = 0;
	uint16_t ucs2str[PTP_MAXSTRLEN+1];
	char *ucs2strp = (char *) ucs2str;
	size_t convlen = strlen(string);

	if (convlen > PTP_MAXSTRLEN) {
		convlen = PTP_MAXSTRLEN;
	}
	/* Cannot exceed 255 (PTP_MAXSTRLEN) since it is a single byte, duh ... */
	memset(ucs2strp, 0, sizeof(ucs2str));  /* XXX: necessary? */
#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_H)
	if (params->cd_locale_to_ucs2 != (iconv_t)-1) {
		size_t nconv;
		size_t convmax = PTP_MAXSTRLEN * 2; /* Includes the terminator */
		char *stringp = string;

		nconv = iconv(params->cd_locale_to_ucs2, &stringp, &convlen,
			&ucs2strp, &convmax);
		if (nconv == (size_t) -1)
			ucs2str[0] = 0x0000U;
	} else
#endif
	{
		unsigned int i;
		if (convlen > PTP_MAXSTRLEN) {
			convlen = PTP_MAXSTRLEN;
		}
		for (i=0;i<convlen;i++) {
			ucs2str[i] = string[i];
		}
		ucs2str[convlen] = 0;
	}
	/*
	 * XXX: isn't packedlen just ( (uint16_t *)ucs2strp - ucs2str )?
	 *      why do we need ucs2strlen()?
	 */
	packedlen = ucs2strlen(ucs2str);
	if (packedlen > PTP_MAXSTRLEN-1) {
		*len=0;
		return;
	}

	/* number of characters including terminating 0 (PTP standard confirmed) */
	htod8a(&data[offset],packedlen+1);
	memcpy(&data[offset+1], &ucs2str[0], packedlen * sizeof(ucs2str[0]));
	htod16a(&data[offset+packedlen*2+1], 0x0000);  /* terminate 0 */

	/* The returned length is in number of characters */
	*len = (uint8_t) packedlen+1;
}

static inline unsigned char *
ptp_get_packed_stringcopy(PTPParams *params, char *string, uint32_t *packed_size)
{
	uint8_t packed[PTP_MAXSTRLEN*2+3], len;
	size_t plen;
	unsigned char *retcopy = NULL;

	if (string == NULL)
	  ptp_pack_string(params, "", (unsigned char*) packed, 0, &len);
	else
	  ptp_pack_string(params, string, (unsigned char*) packed, 0, &len);

	/* returned length is in characters, then one byte for string length */
	plen = len*2 + 1;

	retcopy = malloc(plen);
	if (!retcopy) {
		*packed_size = 0;
		return NULL;
	}
	memcpy(retcopy, packed, plen);
	*packed_size = plen;
	return (retcopy);
}

static inline uint32_t
ptp_unpack_uint32_t_array(PTPParams *params, unsigned char* data, unsigned int offset, unsigned int datalen, uint32_t **array)
{
	uint32_t n, i=0;

	if (!data)
		return 0;

	if (offset >= datalen)
		return 0;

	if (offset + sizeof(uint32_t) > datalen)
		return 0;

	*array = NULL;
	n=dtoh32a(&data[offset]);
	if (n >= UINT_MAX/sizeof(uint32_t))
		return 0;
	if (!n)
		return 0;

	if (offset + sizeof(uint32_t)*(n+1) > datalen) {
		ptp_debug (params ,"array runs over datalen bufferend (%d vs %d)", offset + sizeof(uint32_t)*(n+1) , datalen);
		return 0;
	}

	*array = malloc (n*sizeof(uint32_t));
	if (!*array)
		return 0;
	for (i=0;i<n;i++)
		(*array)[i]=dtoh32a(&data[offset+(sizeof(uint32_t)*(i+1))]);
	return n;
}

static inline uint32_t
ptp_pack_uint32_t_array(PTPParams *params, uint32_t *array, uint32_t arraylen, unsigned char **data )
{
	uint32_t i=0;

	*data = malloc ((arraylen+1)*sizeof(uint32_t));
	if (!*data)
		return 0;
	htod32a(&(*data)[0],arraylen);
	for (i=0;i<arraylen;i++)
		htod32a(&(*data)[sizeof(uint32_t)*(i+1)], array[i]);
	return (arraylen+1)*sizeof(uint32_t);
}

static inline uint32_t
ptp_unpack_uint16_t_array(PTPParams *params, unsigned char* data, unsigned int offset, unsigned int datalen, uint16_t **array)
{
	uint32_t n, i=0;

	if (!data)
		return 0;
	*array = NULL;

	if (datalen - offset < sizeof(uint32_t))
		return 0;
	n=dtoh32a(&data[offset]);

	if (n >= (UINT_MAX - offset - sizeof(uint32_t))/sizeof(uint16_t))
		return 0;
	if (!n)
		return 0;
	if (offset + sizeof(uint32_t) > datalen)
		return 0;
	if (offset + sizeof(uint32_t)+sizeof(uint16_t)*n > datalen) {
		ptp_debug (params ,"array runs over datalen bufferend (%d vs %d)", offset + sizeof(uint32_t)+n*sizeof(uint16_t) , datalen);
		return 0;
	}
	*array = malloc (n*sizeof(uint16_t));
	if (!*array)
		return 0;
	for (i=0;i<n;i++)
		(*array)[i]=dtoh16a(&data[offset+(sizeof(uint16_t)*(i+2))]);
	return n;
}

/* DeviceInfo pack/unpack */

#define PTP_di_StandardVersion		 0
#define PTP_di_VendorExtensionID	 2
#define PTP_di_VendorExtensionVersion	 6
#define PTP_di_VendorExtensionDesc	 8
#define PTP_di_FunctionalMode		 8
#define PTP_di_OperationsSupported	10

static inline int
ptp_unpack_DI (PTPParams *params, unsigned char* data, PTPDeviceInfo *di, unsigned int datalen)
{
	uint8_t len;
	unsigned int totallen;

	if (!data) return 0;
	if (datalen < 12) return 0;
	memset (di, 0, sizeof(*di));
	di->StandardVersion = dtoh16a(&data[PTP_di_StandardVersion]);
	di->VendorExtensionID =
		dtoh32a(&data[PTP_di_VendorExtensionID]);
	di->VendorExtensionVersion =
		dtoh16a(&data[PTP_di_VendorExtensionVersion]);
	if (!ptp_unpack_string(params, data,
		PTP_di_VendorExtensionDesc,
		datalen,
		&len,
		&di->VendorExtensionDesc)
	)
		return 0;
	totallen=len*2+1;
	if (datalen <= totallen + PTP_di_FunctionalMode + sizeof(uint16_t)) {
		ptp_debug (params, "datalen %d <= totallen + PTP_di_FunctionalMode + sizeof(uint16_t) %d", datalen, totallen + PTP_di_FunctionalMode + sizeof(uint16_t));
		return 0;
	}
	di->FunctionalMode =
		dtoh16a(&data[PTP_di_FunctionalMode+totallen]);
	di->OperationsSupported_len = ptp_unpack_uint16_t_array(params, data,
		PTP_di_OperationsSupported+totallen,
		datalen,
		&di->OperationsSupported);
	totallen=totallen+di->OperationsSupported_len*sizeof(uint16_t)+sizeof(uint32_t);
	if (datalen <= totallen+PTP_di_OperationsSupported) {
		ptp_debug (params, "datalen %d <= totallen+PTP_di_OperationsSupported %d 1", datalen, totallen+PTP_di_OperationsSupported);
		return 0;
	}
	di->EventsSupported_len = ptp_unpack_uint16_t_array(params, data,
		PTP_di_OperationsSupported+totallen,
		datalen,
		&di->EventsSupported);
	totallen=totallen+di->EventsSupported_len*sizeof(uint16_t)+sizeof(uint32_t);
	if (datalen <= totallen+PTP_di_OperationsSupported) {
		ptp_debug (params, "datalen %d <= totallen+PTP_di_OperationsSupported %d 2", datalen, totallen+PTP_di_OperationsSupported);
		return 0;
	}
	di->DevicePropertiesSupported_len =
		ptp_unpack_uint16_t_array(params, data,
		PTP_di_OperationsSupported+totallen,
		datalen,
		&di->DevicePropertiesSupported);
	totallen=totallen+di->DevicePropertiesSupported_len*sizeof(uint16_t)+sizeof(uint32_t);
	if (datalen <= totallen+PTP_di_OperationsSupported) {
		ptp_debug (params, "datalen %d <= totallen+PTP_di_OperationsSupported %d 3", datalen, totallen+PTP_di_OperationsSupported);
		return 0;
	}
	di->CaptureFormats_len = ptp_unpack_uint16_t_array(params, data,
		PTP_di_OperationsSupported+totallen,
		datalen,
		&di->CaptureFormats);
	totallen=totallen+di->CaptureFormats_len*sizeof(uint16_t)+sizeof(uint32_t);
	if (datalen <= totallen+PTP_di_OperationsSupported) {
		ptp_debug (params, "datalen %d <= totallen+PTP_di_OperationsSupported %d 4", datalen, totallen+PTP_di_OperationsSupported);
		return 0;
	}
	di->ImageFormats_len = ptp_unpack_uint16_t_array(params, data,
		PTP_di_OperationsSupported+totallen,
		datalen,
		&di->ImageFormats);
	totallen=totallen+di->ImageFormats_len*sizeof(uint16_t)+sizeof(uint32_t);
	if (datalen <= totallen+PTP_di_OperationsSupported) {
		ptp_debug (params, "datalen %d <= totallen+PTP_di_OperationsSupported %d 5", datalen, totallen+PTP_di_OperationsSupported);
		return 0;
	}
	if (!ptp_unpack_string(params, data,
		PTP_di_OperationsSupported+totallen,
		datalen,
		&len,
		&di->Manufacturer)
	)
		return 0;
	totallen+=len*2+1;
	/* be more relaxed ... as these are optional its ok if they are not here */
	if (datalen <= totallen+PTP_di_OperationsSupported) {
		ptp_debug (params, "datalen %d <= totallen+PTP_di_OperationsSupported %d 6", datalen, totallen+PTP_di_OperationsSupported);
		return 1;
	}
	if (!ptp_unpack_string(params, data,
		PTP_di_OperationsSupported+totallen,
		datalen,
		&len,
		&di->Model)
	)
		return 1;
	totallen+=len*2+1;
	/* be more relaxed ... as these are optional its ok if they are not here */
	if (datalen <= totallen+PTP_di_OperationsSupported) {
		ptp_debug (params, "datalen %d <= totallen+PTP_di_OperationsSupported %d 7", datalen, totallen+PTP_di_OperationsSupported);
		return 1;
	}
	if (!ptp_unpack_string(params, data,
		PTP_di_OperationsSupported+totallen,
		datalen,
		&len,
		&di->DeviceVersion)
	)
		return 1;
	totallen+=len*2+1;
	/* be more relaxed ... as these are optional its ok if they are not here */
	if (datalen <= totallen+PTP_di_OperationsSupported) {
		ptp_debug (params, "datalen %d <= totallen+PTP_di_OperationsSupported %d 8", datalen, totallen+PTP_di_OperationsSupported);
		return 1;
	}
	if (!ptp_unpack_string(params, data,
		PTP_di_OperationsSupported+totallen,
		datalen,
		&len,
		&di->SerialNumber)
	)
		return 1;
	return 1;
}

inline static void
ptp_free_DI (PTPDeviceInfo *di) {
	free (di->SerialNumber);
	free (di->DeviceVersion);
	free (di->Model);
	free (di->Manufacturer);
	free (di->ImageFormats);
	free (di->CaptureFormats);
	free (di->VendorExtensionDesc);
	free (di->OperationsSupported);
	free (di->EventsSupported);
	free (di->DevicePropertiesSupported);
	memset(di, 0, sizeof(*di));
}

/* EOS Device Info unpack */
static inline int
ptp_unpack_EOS_DI (PTPParams *params, unsigned char* data, PTPCanonEOSDeviceInfo *di, unsigned int datalen)
{
	unsigned int totallen = 4;

	memset (di,0, sizeof(*di));
	if (datalen < 8) return 0;

	/* uint32_t struct len - ignore */
	di->EventsSupported_len = ptp_unpack_uint32_t_array(params, data,
		totallen, datalen, &di->EventsSupported);
	if (!di->EventsSupported) return 0;
	totallen += di->EventsSupported_len*sizeof(uint32_t)+4;
	if (totallen >= datalen) return 0;

	di->DevicePropertiesSupported_len = ptp_unpack_uint32_t_array(params, data,
		totallen, datalen, &di->DevicePropertiesSupported);
	if (!di->DevicePropertiesSupported) return 0;
	totallen += di->DevicePropertiesSupported_len*sizeof(uint32_t)+4;
	if (totallen >= datalen) return 0;

	di->unk_len = ptp_unpack_uint32_t_array(params, data,
		totallen, datalen, &di->unk);
	if (!di->unk) return 0;
	totallen += di->unk_len*sizeof(uint32_t)+4;
	return 1;
}

static inline void
ptp_free_EOS_DI (PTPCanonEOSDeviceInfo *di)
{
	free (di->EventsSupported);
	free (di->DevicePropertiesSupported);
	free (di->unk);
}

/* ObjectHandles array pack/unpack */

#define PTP_oh				 0

static inline void
ptp_unpack_OH (PTPParams *params, unsigned char* data, PTPObjectHandles *oh, unsigned int len)
{
	if (len) {
		oh->n = ptp_unpack_uint32_t_array(params, data, PTP_oh, len, &oh->Handler);
	} else {
		oh->n = 0;
		oh->Handler = NULL;
	}
}

/* StoreIDs array pack/unpack */

#define PTP_sids			 0

static inline void
ptp_unpack_SIDs (PTPParams *params, unsigned char* data, PTPStorageIDs *sids, unsigned int len)
{
	sids->n = 0;
	sids->Storage = NULL;

	if (!data || !len)
		return;

	sids->n = ptp_unpack_uint32_t_array(params, data, PTP_sids, len, &sids->Storage);
}

/* StorageInfo pack/unpack */

#define PTP_si_StorageType		 0
#define PTP_si_FilesystemType		 2
#define PTP_si_AccessCapability		 4
#define PTP_si_MaxCapability		 6
#define PTP_si_FreeSpaceInBytes		14
#define PTP_si_FreeSpaceInImages	22
#define PTP_si_StorageDescription	26

static inline int
ptp_unpack_SI (PTPParams *params, unsigned char* data, PTPStorageInfo *si, unsigned int len)
{
	uint8_t storagedescriptionlen;

	if (len < 26) return 0;
	si->StorageType=dtoh16a(&data[PTP_si_StorageType]);
	si->FilesystemType=dtoh16a(&data[PTP_si_FilesystemType]);
	si->AccessCapability=dtoh16a(&data[PTP_si_AccessCapability]);
	si->MaxCapability=dtoh64a(&data[PTP_si_MaxCapability]);
	si->FreeSpaceInBytes=dtoh64a(&data[PTP_si_FreeSpaceInBytes]);
	si->FreeSpaceInImages=dtoh32a(&data[PTP_si_FreeSpaceInImages]);

	/* FIXME: check more lengths here */
	if (!ptp_unpack_string(params, data,
		PTP_si_StorageDescription,
		len,
		&storagedescriptionlen,
		&si->StorageDescription)
	)
		return 0;

	if (!ptp_unpack_string(params, data,
		PTP_si_StorageDescription+storagedescriptionlen*2+1,
		len,
		&storagedescriptionlen,
		&si->VolumeLabel)) {
		ptp_debug(params, "could not unpack storage description");
		return 0;
	}
	return 1;
}

/* ObjectInfo pack/unpack */

#define PTP_oi_StorageID		 0
#define PTP_oi_ObjectFormat		 4
#define PTP_oi_ProtectionStatus		 6
#define PTP_oi_ObjectCompressedSize	 8
#define PTP_oi_ThumbFormat		12
#define PTP_oi_ThumbCompressedSize	14
#define PTP_oi_ThumbPixWidth		18
#define PTP_oi_ThumbPixHeight		22
#define PTP_oi_ImagePixWidth		26
#define PTP_oi_ImagePixHeight		30
#define PTP_oi_ImageBitDepth		34
#define PTP_oi_ParentObject		38
#define PTP_oi_AssociationType		42
#define PTP_oi_AssociationDesc		44
#define PTP_oi_SequenceNumber		48
#define PTP_oi_filenamelen		52
#define PTP_oi_Filename			53

/* the max length assuming zero length dates. We have need 3 */
/* bytes for these. */
#define PTP_oi_MaxLen PTP_oi_Filename+(PTP_MAXSTRLEN+1)*2+3

static inline uint32_t
ptp_pack_OI (PTPParams *params, PTPObjectInfo *oi, unsigned char** oidataptr)
{
	unsigned char* oidata;
	uint8_t filenamelen;
	uint8_t capturedatelen=0;
	/* let's allocate some memory first; correct assuming zero length dates */
	oidata=malloc(PTP_oi_MaxLen + params->ocs64*4);
	*oidataptr=oidata;
	/* the caller should free it after use! */
#if 0
	char *capture_date="20020101T010101"; /* XXX Fake date */
#endif
	memset (oidata, 0, PTP_oi_MaxLen + params->ocs64*4);
	htod32a(&oidata[PTP_oi_StorageID],oi->StorageID);
	htod16a(&oidata[PTP_oi_ObjectFormat],oi->ObjectFormat);
	htod16a(&oidata[PTP_oi_ProtectionStatus],oi->ProtectionStatus);
	htod32a(&oidata[PTP_oi_ObjectCompressedSize],oi->ObjectCompressedSize);
	if (params->ocs64)
		oidata += 4;
	htod16a(&oidata[PTP_oi_ThumbFormat],oi->ThumbFormat);
	htod32a(&oidata[PTP_oi_ThumbCompressedSize],oi->ThumbCompressedSize);
	htod32a(&oidata[PTP_oi_ThumbPixWidth],oi->ThumbPixWidth);
	htod32a(&oidata[PTP_oi_ThumbPixHeight],oi->ThumbPixHeight);
	htod32a(&oidata[PTP_oi_ImagePixWidth],oi->ImagePixWidth);
	htod32a(&oidata[PTP_oi_ImagePixHeight],oi->ImagePixHeight);
	htod32a(&oidata[PTP_oi_ImageBitDepth],oi->ImageBitDepth);
	htod32a(&oidata[PTP_oi_ParentObject],oi->ParentObject);
	htod16a(&oidata[PTP_oi_AssociationType],oi->AssociationType);
	htod32a(&oidata[PTP_oi_AssociationDesc],oi->AssociationDesc);
	htod32a(&oidata[PTP_oi_SequenceNumber],oi->SequenceNumber);

	ptp_pack_string(params, oi->Filename, oidata, PTP_oi_filenamelen, &filenamelen);
/*
	filenamelen=(uint8_t)strlen(oi->Filename);
	htod8a(&req->data[PTP_oi_filenamelen],filenamelen+1);
	for (i=0;i<filenamelen && i< PTP_MAXSTRLEN; i++) {
		req->data[PTP_oi_Filename+i*2]=oi->Filename[i];
	}
*/
	/*
	 *XXX Fake date.
	 * for example Kodak sets Capture date on the basis of EXIF data.
	 * Spec says that this field is from perspective of Initiator.
	 */
#if 0	/* seems now we don't need any data packed in OI dataset... for now ;)*/
	capturedatelen=strlen(capture_date);
	htod8a(&data[PTP_oi_Filename+(filenamelen+1)*2],
		capturedatelen+1);
	for (i=0;i<capturedatelen && i< PTP_MAXSTRLEN; i++) {
		data[PTP_oi_Filename+(i+filenamelen+1)*2+1]=capture_date[i];
	}
	htod8a(&data[PTP_oi_Filename+(filenamelen+capturedatelen+2)*2+1],
		capturedatelen+1);
	for (i=0;i<capturedatelen && i< PTP_MAXSTRLEN; i++) {
		data[PTP_oi_Filename+(i+filenamelen+capturedatelen+2)*2+2]=
		  capture_date[i];
	}
#endif
	/* XXX this function should return dataset length */
	return (PTP_oi_Filename+filenamelen*2+(capturedatelen+1)*3)+params->ocs64*4;
}

static time_t
ptp_unpack_PTPTIME (const char *str) {
	char ptpdate[40];
	char tmp[5];
	size_t  ptpdatelen;
	struct tm tm;

	if (!str)
		return 0;
	ptpdatelen = strlen(str);
	if (ptpdatelen >= sizeof (ptpdate)) {
		/*ptp_debug (params ,"datelen is larger then size of buffer", ptpdatelen, (int)sizeof(ptpdate));*/
		return 0;
	}
	if (ptpdatelen<15) {
		/*ptp_debug (params ,"datelen is less than 15 (%d)", ptpdatelen);*/
		return 0;
	}
	strncpy (ptpdate, str, sizeof(ptpdate));
	ptpdate[sizeof(ptpdate) - 1] = '\0';

	memset(&tm,0,sizeof(tm));
	strncpy (tmp, ptpdate, 4);
	tmp[4] = 0;
	tm.tm_year=atoi (tmp) - 1900;
	strncpy (tmp, ptpdate + 4, 2);
	tmp[2] = 0;
	tm.tm_mon = atoi (tmp) - 1;
	strncpy (tmp, ptpdate + 6, 2);
	tmp[2] = 0;
	tm.tm_mday = atoi (tmp);
	strncpy (tmp, ptpdate + 9, 2);
	tmp[2] = 0;
	tm.tm_hour = atoi (tmp);
	strncpy (tmp, ptpdate + 11, 2);
	tmp[2] = 0;
	tm.tm_min = atoi (tmp);
	strncpy (tmp, ptpdate + 13, 2);
	tmp[2] = 0;
	tm.tm_sec = atoi (tmp);
	tm.tm_isdst = -1;
	return mktime (&tm);
}

static inline void
ptp_unpack_OI (PTPParams *params, unsigned char* data, PTPObjectInfo *oi, unsigned int len)
{
	uint8_t filenamelen;
	uint8_t capturedatelen;
	char *capture_date;

	if (len < PTP_oi_SequenceNumber)
		return;

	oi->Filename = oi->Keywords = NULL;

	/* FIXME: also handle length with all the strings at the end */
	oi->StorageID=dtoh32a(&data[PTP_oi_StorageID]);
	oi->ObjectFormat=dtoh16a(&data[PTP_oi_ObjectFormat]);
	oi->ProtectionStatus=dtoh16a(&data[PTP_oi_ProtectionStatus]);
	oi->ObjectCompressedSize=dtoh32a(&data[PTP_oi_ObjectCompressedSize]);

	/* Stupid Samsung Galaxy developers emit a 64bit objectcompressedsize */
	if ((data[PTP_oi_filenamelen] == 0) && (data[PTP_oi_filenamelen+4] != 0)) {
		ptp_debug (params, "objectsize 64bit detected!");
		params->ocs64 = 1;
		data += 4;
		len -= 4;
	}
	oi->ThumbFormat=dtoh16a(&data[PTP_oi_ThumbFormat]);
	oi->ThumbCompressedSize=dtoh32a(&data[PTP_oi_ThumbCompressedSize]);
	oi->ThumbPixWidth=dtoh32a(&data[PTP_oi_ThumbPixWidth]);
	oi->ThumbPixHeight=dtoh32a(&data[PTP_oi_ThumbPixHeight]);
	oi->ImagePixWidth=dtoh32a(&data[PTP_oi_ImagePixWidth]);
	oi->ImagePixHeight=dtoh32a(&data[PTP_oi_ImagePixHeight]);
	oi->ImageBitDepth=dtoh32a(&data[PTP_oi_ImageBitDepth]);
	oi->ParentObject=dtoh32a(&data[PTP_oi_ParentObject]);
	oi->AssociationType=dtoh16a(&data[PTP_oi_AssociationType]);
	oi->AssociationDesc=dtoh32a(&data[PTP_oi_AssociationDesc]);
	oi->SequenceNumber=dtoh32a(&data[PTP_oi_SequenceNumber]);

	ptp_unpack_string(params, data, PTP_oi_filenamelen, len, &filenamelen, &oi->Filename);
	ptp_unpack_string(params, data, PTP_oi_filenamelen+filenamelen*2+1, len, &capturedatelen, &capture_date);
	/* subset of ISO 8601, without '.s' tenths of second and
	 * time zone
	 */
	oi->CaptureDate = ptp_unpack_PTPTIME(capture_date);
	free(capture_date);

	/* now the modification date ... */
	ptp_unpack_string(params, data,
		PTP_oi_filenamelen+filenamelen*2
		+capturedatelen*2+2, len, &capturedatelen, &capture_date
	);
	oi->ModificationDate = ptp_unpack_PTPTIME(capture_date);
	free(capture_date);
}

/* Custom Type Value Assignement (without Length) macro frequently used below */
#define CTVAL(target,func) {			\
	if (total - *offset < sizeof(target))	\
		return 0;			\
	target = func(&data[*offset]);		\
	*offset += sizeof(target);		\
}

#define RARR(val,member,func)	{			\
	unsigned int n,j;				\
	if (total - *offset < sizeof(uint32_t))		\
		return 0;				\
	n = dtoh32a (&data[*offset]);			\
	*offset += sizeof(uint32_t);			\
							\
	if (n >= UINT_MAX/sizeof(val->a.v[0]))		\
		return 0;				\
	if (n > (total - (*offset))/sizeof(val->a.v[0].member))\
		return 0;				\
	val->a.count = n;				\
	val->a.v = malloc(sizeof(val->a.v[0])*n);	\
	if (!val->a.v) return 0;			\
	for (j=0;j<n;j++)				\
		CTVAL(val->a.v[j].member, func);	\
}

static inline unsigned int
ptp_unpack_DPV (
	PTPParams *params, unsigned char* data, unsigned int *offset, unsigned int total,
	PTPPropertyValue* value, uint16_t datatype
) {
	if (*offset >= total)	/* we are at the end or over the end of the buffer */
		return 0;

	switch (datatype) {
	case PTP_DTC_INT8:
		CTVAL(value->i8,dtoh8a);
		break;
	case PTP_DTC_UINT8:
		CTVAL(value->u8,dtoh8a);
		break;
	case PTP_DTC_INT16:
		CTVAL(value->i16,dtoh16a);
		break;
	case PTP_DTC_UINT16:
		CTVAL(value->u16,dtoh16a);
		break;
	case PTP_DTC_INT32:
		CTVAL(value->i32,dtoh32a);
		break;
	case PTP_DTC_UINT32:
		CTVAL(value->u32,dtoh32a);
		break;
	case PTP_DTC_INT64:
		CTVAL(value->i64,dtoh64a);
		break;
	case PTP_DTC_UINT64:
		CTVAL(value->u64,dtoh64a);
		break;

	case PTP_DTC_UINT128:
		*offset += 16;
		/*fprintf(stderr,"unhandled unpack of uint128n");*/
		break;
	case PTP_DTC_INT128:
		*offset += 16;
		/*fprintf(stderr,"unhandled unpack of int128n");*/
		break;



	case PTP_DTC_AINT8:
		RARR(value,i8,dtoh8a);
		break;
	case PTP_DTC_AUINT8:
		RARR(value,u8,dtoh8a);
		break;
	case PTP_DTC_AUINT16:
		RARR(value,u16,dtoh16a);
		break;
	case PTP_DTC_AINT16:
		RARR(value,i16,dtoh16a);
		break;
	case PTP_DTC_AUINT32:
		RARR(value,u32,dtoh32a);
		break;
	case PTP_DTC_AINT32:
		RARR(value,i32,dtoh32a);
		break;
	case PTP_DTC_AUINT64:
		RARR(value,u64,dtoh64a);
		break;
	case PTP_DTC_AINT64:
		RARR(value,i64,dtoh64a);
		break;
	/* XXX: other int types are unimplemented */
	/* XXX: other int arrays are unimplemented also */
	case PTP_DTC_STR: {
		uint8_t len;
		/* XXX: max size */

		if (*offset >= total+1)
			return 0;

		if (!ptp_unpack_string(params,data,*offset,total,&len,&value->str))
			return 0;
		*offset += len*2+1;
		break;
	}
	default:
		return 0;
	}
	return 1;
}

/* Device Property pack/unpack */
#define PTP_dpd_DevicePropertyCode	0
#define PTP_dpd_DataType		2
#define PTP_dpd_GetSet			4
#define PTP_dpd_FactoryDefaultValue	5

static inline int
ptp_unpack_DPD (PTPParams *params, unsigned char* data, PTPDevicePropDesc *dpd, unsigned int dpdlen, unsigned int *newoffset)
{
	unsigned int offset = 0, ret;

	*newoffset = 0;

	memset (dpd, 0, sizeof(*dpd));
	if (dpdlen <= 5)
		return 0;
	dpd->DevicePropertyCode=dtoh16a(&data[PTP_dpd_DevicePropertyCode]);
	dpd->DataType=dtoh16a(&data[PTP_dpd_DataType]);
	dpd->GetSet=dtoh8a(&data[PTP_dpd_GetSet]);
	dpd->FormFlag=PTP_DPFF_None;

	offset = PTP_dpd_FactoryDefaultValue;
	ret = ptp_unpack_DPV (params, data, &offset, dpdlen, &dpd->FactoryDefaultValue, dpd->DataType);
	if (!ret) goto outofmemory;
	if ((dpd->DataType == PTP_DTC_STR) && (offset == dpdlen)) {
		*newoffset = offset;
		return 1;
	}
	ret = ptp_unpack_DPV (params, data, &offset, dpdlen, &dpd->CurrentValue, dpd->DataType);
	if (!ret) goto outofmemory;

	/* if offset==0 then Data Type format is not supported by this
	   code or the Data Type is a string (with two empty strings as
	   values). In both cases Form Flag should be set to 0x00 and FORM is
	   not present. */

	if (offset + sizeof(uint8_t) > dpdlen) {
		*newoffset = offset;
		return 1;
	}

	dpd->FormFlag=dtoh8a(&data[offset]);
	offset+=sizeof(uint8_t);

	switch (dpd->FormFlag) {
	case PTP_DPFF_Range:
		ret = ptp_unpack_DPV (params, data, &offset, dpdlen, &dpd->FORM.Range.MinimumValue, dpd->DataType);
		if (!ret) goto outofmemory;
		ret = ptp_unpack_DPV (params, data, &offset, dpdlen, &dpd->FORM.Range.MaximumValue, dpd->DataType);
		if (!ret) goto outofmemory;
		ret = ptp_unpack_DPV (params, data, &offset, dpdlen, &dpd->FORM.Range.StepSize, dpd->DataType);
		if (!ret) goto outofmemory;
		break;
	case PTP_DPFF_Enumeration: {
		int i;
#define N	dpd->FORM.Enum.NumberOfValues

		if (offset + sizeof(uint16_t) > dpdlen) goto outofmemory;

		N = dtoh16a(&data[offset]);
		offset+=sizeof(uint16_t);
		dpd->FORM.Enum.SupportedValue = malloc(N*sizeof(dpd->FORM.Enum.SupportedValue[0]));
		if (!dpd->FORM.Enum.SupportedValue)
			goto outofmemory;

		memset (dpd->FORM.Enum.SupportedValue,0 , N*sizeof(dpd->FORM.Enum.SupportedValue[0]));
		for (i=0;i<N;i++) {
			ret = ptp_unpack_DPV (params, data, &offset, dpdlen, &dpd->FORM.Enum.SupportedValue[i], dpd->DataType);

			/* Slightly different handling here. The HP PhotoSmart 120
			 * specifies an enumeration with N in wrong endian
			 * 00 01 instead of 01 00, so we count the enum just until the
			 * the end of the packet.
			 */
			if (!ret) {
				if (!i)
					goto outofmemory;
				dpd->FORM.Enum.NumberOfValues = i;
				break;
			}
		}
		}
	}
#undef N
	*newoffset = offset;
	return 1;
outofmemory:
	ptp_free_devicepropdesc(dpd);
	return 0;
}

/* Device Property pack/unpack */
#define PTP_dpd_Sony_DevicePropertyCode		0
#define PTP_dpd_Sony_DataType			2
#define PTP_dpd_Sony_ChangeMethod		4
#define PTP_dpd_Sony_GetSet			5
#define PTP_dpd_Sony_FactoryDefaultValue	6

static inline int
ptp_unpack_Sony_DPD (PTPParams *params, unsigned char* data, PTPDevicePropDesc *dpd, unsigned int dpdlen, unsigned int *poffset)
{
	unsigned int ret;
	unsigned int changemethod, getset;

	memset (dpd, 0, sizeof(*dpd));
	dpd->DevicePropertyCode=dtoh16a(&data[PTP_dpd_Sony_DevicePropertyCode]);
	dpd->DataType=dtoh16a(&data[PTP_dpd_Sony_DataType]);

	changemethod = dtoh8a(&data[PTP_dpd_Sony_ChangeMethod]);
	getset = dtoh8a(&data[PTP_dpd_Sony_GetSet]);

	ptp_debug (params, "prop 0x%04x, datatype 0x%04x, changemethod %d getset %d", dpd->DevicePropertyCode, dpd->DataType, changemethod, getset);
	switch (getset) {
	case 0:	/* read only */
	case 2: /* camera side only */
		dpd->GetSet=0;
		break;
	case 1: /* writeable */
		dpd->GetSet=1;
		break;
	default: /* values over 128 ... likely action triggers */
		dpd->GetSet=1;
		break;
	}
	dpd->FormFlag=PTP_DPFF_None;

	*poffset = PTP_dpd_Sony_FactoryDefaultValue;
	ret = ptp_unpack_DPV (params, data, poffset, dpdlen, &dpd->FactoryDefaultValue, dpd->DataType);
	if (!ret) goto outofmemory;
	if ((dpd->DataType == PTP_DTC_STR) && (*poffset == dpdlen))
		return 1;
	ret = ptp_unpack_DPV (params, data, poffset, dpdlen, &dpd->CurrentValue, dpd->DataType);
	if (!ret) goto outofmemory;

	/* if offset==0 then Data Type format is not supported by this
	   code or the Data Type is a string (with two empty strings as
	   values). In both cases Form Flag should be set to 0x00 and FORM is
	   not present. */

	if (*poffset==PTP_dpd_Sony_FactoryDefaultValue)
		return 1;

	dpd->FormFlag=dtoh8a(&data[*poffset]);
	*poffset+=sizeof(uint8_t);

	switch (dpd->FormFlag) {
	case PTP_DPFF_Range:
		ret = ptp_unpack_DPV (params, data, poffset, dpdlen, &dpd->FORM.Range.MinimumValue, dpd->DataType);
		if (!ret) goto outofmemory;
		ret = ptp_unpack_DPV (params, data, poffset, dpdlen, &dpd->FORM.Range.MaximumValue, dpd->DataType);
		if (!ret) goto outofmemory;
		ret = ptp_unpack_DPV (params, data, poffset, dpdlen, &dpd->FORM.Range.StepSize, dpd->DataType);
		if (!ret) goto outofmemory;
		break;
	case PTP_DPFF_Enumeration: {
		int i;
#define N	dpd->FORM.Enum.NumberOfValues
		N = dtoh16a(&data[*poffset]);
		*poffset+=sizeof(uint16_t);
		dpd->FORM.Enum.SupportedValue = malloc(N*sizeof(dpd->FORM.Enum.SupportedValue[0]));
		if (!dpd->FORM.Enum.SupportedValue)
			goto outofmemory;

		memset (dpd->FORM.Enum.SupportedValue,0 , N*sizeof(dpd->FORM.Enum.SupportedValue[0]));
		for (i=0;i<N;i++) {
			ret = ptp_unpack_DPV (params, data, poffset, dpdlen, &dpd->FORM.Enum.SupportedValue[i], dpd->DataType);

			/* Slightly different handling here. The HP PhotoSmart 120
			 * specifies an enumeration with N in wrong endian
			 * 00 01 instead of 01 00, so we count the enum just until the
			 * the end of the packet.
			 */
			if (!ret) {
				if (!i)
					goto outofmemory;
				dpd->FORM.Enum.NumberOfValues = i;
				break;
			}
		}
		}
	}
#undef N
	return 1;
outofmemory:
	ptp_free_devicepropdesc(dpd);
	return 0;
}

static inline void
duplicate_PropertyValue (const PTPPropertyValue *src, PTPPropertyValue *dst, uint16_t type) {
	if (type == PTP_DTC_STR) {
		if (src->str)
			dst->str = strdup(src->str);
		else
			dst->str = NULL;
		return;
	}

	if (type & PTP_DTC_ARRAY_MASK) {
		unsigned int i;

		dst->a.count = src->a.count;
		dst->a.v = malloc (sizeof(src->a.v[0])*src->a.count);
		for (i=0;i<src->a.count;i++)
			duplicate_PropertyValue (&src->a.v[i], &dst->a.v[i], type & ~PTP_DTC_ARRAY_MASK);
		return;
	}
	switch (type & ~PTP_DTC_ARRAY_MASK) {
	case PTP_DTC_INT8:	dst->i8 = src->i8; break;
	case PTP_DTC_UINT8:	dst->u8 = src->u8; break;
	case PTP_DTC_INT16:	dst->i16 = src->i16; break;
	case PTP_DTC_UINT16:	dst->u16 = src->u16; break;
	case PTP_DTC_INT32:	dst->i32 = src->i32; break;
	case PTP_DTC_UINT32:	dst->u32 = src->u32; break;
	case PTP_DTC_UINT64:	dst->u64 = src->u64; break;
	case PTP_DTC_INT64:	dst->i64 = src->i64; break;
#if 0
	case PTP_DTC_INT128:	dst->i128 = src->i128; break;
	case PTP_DTC_UINT128:	dst->u128 = src->u128; break;
#endif
	default:		break;
	}
	return;
}

static inline void
duplicate_DevicePropDesc(const PTPDevicePropDesc *src, PTPDevicePropDesc *dst) {
	int i;

	dst->DevicePropertyCode	= src->DevicePropertyCode;
	dst->DataType		= src->DataType;
	dst->GetSet		= src->GetSet;

	duplicate_PropertyValue (&src->FactoryDefaultValue, &dst->FactoryDefaultValue, src->DataType);
	duplicate_PropertyValue (&src->CurrentValue, &dst->CurrentValue, src->DataType);

	dst->FormFlag		= src->FormFlag;
	switch (src->FormFlag) {
	case PTP_DPFF_Range:
		duplicate_PropertyValue (&src->FORM.Range.MinimumValue, &dst->FORM.Range.MinimumValue, src->DataType);
		duplicate_PropertyValue (&src->FORM.Range.MaximumValue, &dst->FORM.Range.MaximumValue, src->DataType);
		duplicate_PropertyValue (&src->FORM.Range.StepSize,     &dst->FORM.Range.StepSize,     src->DataType);
		break;
	case PTP_DPFF_Enumeration:
		dst->FORM.Enum.NumberOfValues = src->FORM.Enum.NumberOfValues;
		dst->FORM.Enum.SupportedValue = malloc (sizeof(dst->FORM.Enum.SupportedValue[0])*src->FORM.Enum.NumberOfValues);
		for (i = 0; i<src->FORM.Enum.NumberOfValues ; i++)
			duplicate_PropertyValue (&src->FORM.Enum.SupportedValue[i], &dst->FORM.Enum.SupportedValue[i], src->DataType);
		break;
	case PTP_DPFF_None:
		break;
	}
}

#define PTP_opd_ObjectPropertyCode	0
#define PTP_opd_DataType		2
#define PTP_opd_GetSet			4
#define PTP_opd_FactoryDefaultValue	5

static inline int
ptp_unpack_OPD (PTPParams *params, unsigned char* data, PTPObjectPropDesc *opd, unsigned int opdlen)
{
	unsigned int offset=0, ret;
	uint8_t	len;

	memset (opd, 0, sizeof(*opd));

	if (opdlen < 5)
		return 0;

	opd->ObjectPropertyCode=dtoh16a(&data[PTP_opd_ObjectPropertyCode]);
	opd->DataType=dtoh16a(&data[PTP_opd_DataType]);
	opd->GetSet=dtoh8a(&data[PTP_opd_GetSet]);

	offset = PTP_opd_FactoryDefaultValue;
	ret = ptp_unpack_DPV (params, data, &offset, opdlen, &opd->FactoryDefaultValue, opd->DataType);
	if (!ret) goto outofmemory;

	if (offset + sizeof(uint32_t) > opdlen) goto outofmemory;
	opd->GroupCode=dtoh32a(&data[offset]);
	offset+=sizeof(uint32_t);

	if (offset + sizeof(uint8_t) > opdlen) goto outofmemory;
	opd->FormFlag=dtoh8a(&data[offset]);
	offset+=sizeof(uint8_t);

	switch (opd->FormFlag) {
	case PTP_OPFF_Range:
		ret = ptp_unpack_DPV (params, data, &offset, opdlen, &opd->FORM.Range.MinimumValue, opd->DataType);
		if (!ret) goto outofmemory;
		ret = ptp_unpack_DPV (params, data, &offset, opdlen, &opd->FORM.Range.MaximumValue, opd->DataType);
		if (!ret) goto outofmemory;
		ret = ptp_unpack_DPV (params, data, &offset, opdlen, &opd->FORM.Range.StepSize, opd->DataType);
		if (!ret) goto outofmemory;
		break;
	case PTP_OPFF_Enumeration: {
		unsigned int i;
#define N	opd->FORM.Enum.NumberOfValues

		if (offset + sizeof(uint16_t) > opdlen) goto outofmemory;
		N = dtoh16a(&data[offset]);
		offset+=sizeof(uint16_t);

		opd->FORM.Enum.SupportedValue = malloc(N*sizeof(opd->FORM.Enum.SupportedValue[0]));
		if (!opd->FORM.Enum.SupportedValue)
			goto outofmemory;

		memset (opd->FORM.Enum.SupportedValue,0 , N*sizeof(opd->FORM.Enum.SupportedValue[0]));
		for (i=0;i<N;i++) {
			ret = ptp_unpack_DPV (params, data, &offset, opdlen, &opd->FORM.Enum.SupportedValue[i], opd->DataType);

			/* Slightly different handling here. The HP PhotoSmart 120
			 * specifies an enumeration with N in wrong endian
			 * 00 01 instead of 01 00, so we count the enum just until the
			 * the end of the packet.
			 */
			if (!ret) {
				if (!i)
					goto outofmemory;
				opd->FORM.Enum.NumberOfValues = i;
				break;
			}
		}
#undef N
		}
		break;
	case PTP_OPFF_DateTime:
		if (!ptp_unpack_string(params, data, offset, opdlen, &len, &opd->FORM.DateTime.String))
			opd->FORM.DateTime.String = NULL;
		offset += 2*len+1; /* offset not used afterwards anymore */
		break;
	case PTP_OPFF_RegularExpression:
		if (!ptp_unpack_string(params, data, offset, opdlen, &len, &opd->FORM.RegularExpression.String))
			opd->FORM.RegularExpression.String = NULL;
		offset += 2*len+1; /* offset not used afterwards anymore */
		break;
	case PTP_OPFF_FixedLengthArray:
		if (offset + sizeof(uint16_t) > opdlen) goto outofmemory;
		opd->FORM.FixedLengthArray.NumberOfValues = dtoh16a(&data[offset]);
		offset += sizeof(uint16_t); /* offset not used afterwards anymore */
		break;
	case PTP_OPFF_ByteArray:
		if (offset + sizeof(uint16_t) > opdlen) goto outofmemory;
		opd->FORM.ByteArray.NumberOfValues = dtoh16a(&data[offset]);
		offset += sizeof(uint16_t); /* offset not used afterwards anymore */
		break;
	case PTP_OPFF_LongString:
		break;
	}
	return 1;
outofmemory:
	ptp_free_objectpropdesc(opd);
	return 0;
}

static inline uint32_t
ptp_pack_DPV (PTPParams *params, PTPPropertyValue* value, unsigned char** dpvptr, uint16_t datatype)
{
	unsigned char* dpv=NULL;
	uint32_t size=0;
	unsigned int i;

	switch (datatype) {
	case PTP_DTC_INT8:
		size=sizeof(int8_t);
		dpv=malloc(size);
		htod8a(dpv,value->i8);
		break;
	case PTP_DTC_UINT8:
		size=sizeof(uint8_t);
		dpv=malloc(size);
		htod8a(dpv,value->u8);
		break;
	case PTP_DTC_INT16:
		size=sizeof(int16_t);
		dpv=malloc(size);
		htod16a(dpv,value->i16);
		break;
	case PTP_DTC_UINT16:
		size=sizeof(uint16_t);
		dpv=malloc(size);
		htod16a(dpv,value->u16);
		break;
	case PTP_DTC_INT32:
		size=sizeof(int32_t);
		dpv=malloc(size);
		htod32a(dpv,value->i32);
		break;
	case PTP_DTC_UINT32:
		size=sizeof(uint32_t);
		dpv=malloc(size);
		htod32a(dpv,value->u32);
		break;
	case PTP_DTC_INT64:
		size=sizeof(int64_t);
		dpv=malloc(size);
		htod64a(dpv,value->i64);
		break;
	case PTP_DTC_UINT64:
		size=sizeof(uint64_t);
		dpv=malloc(size);
		htod64a(dpv,value->u64);
		break;
	case PTP_DTC_AUINT8:
		size=sizeof(uint32_t)+value->a.count*sizeof(uint8_t);
		dpv=malloc(size);
		htod32a(dpv,value->a.count);
		for (i=0;i<value->a.count;i++)
			htod8a(&dpv[sizeof(uint32_t)+i*sizeof(uint8_t)],value->a.v[i].u8);
		break;
	case PTP_DTC_AINT8:
		size=sizeof(uint32_t)+value->a.count*sizeof(int8_t);
		dpv=malloc(size);
		htod32a(dpv,value->a.count);
		for (i=0;i<value->a.count;i++)
			htod8a(&dpv[sizeof(uint32_t)+i*sizeof(int8_t)],value->a.v[i].i8);
		break;
	case PTP_DTC_AUINT16:
		size=sizeof(uint32_t)+value->a.count*sizeof(uint16_t);
		dpv=malloc(size);
		htod32a(dpv,value->a.count);
		for (i=0;i<value->a.count;i++)
			htod16a(&dpv[sizeof(uint32_t)+i*sizeof(uint16_t)],value->a.v[i].u16);
		break;
	case PTP_DTC_AINT16:
		size=sizeof(uint32_t)+value->a.count*sizeof(int16_t);
		dpv=malloc(size);
		htod32a(dpv,value->a.count);
		for (i=0;i<value->a.count;i++)
			htod16a(&dpv[sizeof(uint32_t)+i*sizeof(int16_t)],value->a.v[i].i16);
		break;
	case PTP_DTC_AUINT32:
		size=sizeof(uint32_t)+value->a.count*sizeof(uint32_t);
		dpv=malloc(size);
		htod32a(dpv,value->a.count);
		for (i=0;i<value->a.count;i++)
			htod32a(&dpv[sizeof(uint32_t)+i*sizeof(uint32_t)],value->a.v[i].u32);
		break;
	case PTP_DTC_AINT32:
		size=sizeof(uint32_t)+value->a.count*sizeof(int32_t);
		dpv=malloc(size);
		htod32a(dpv,value->a.count);
		for (i=0;i<value->a.count;i++)
			htod32a(&dpv[sizeof(uint32_t)+i*sizeof(int32_t)],value->a.v[i].i32);
		break;
	case PTP_DTC_AUINT64:
		size=sizeof(uint32_t)+value->a.count*sizeof(uint64_t);
		dpv=malloc(size);
		htod32a(dpv,value->a.count);
		for (i=0;i<value->a.count;i++)
			htod64a(&dpv[sizeof(uint32_t)+i*sizeof(uint64_t)],value->a.v[i].u64);
		break;
	case PTP_DTC_AINT64:
		size=sizeof(uint32_t)+value->a.count*sizeof(int64_t);
		dpv=malloc(size);
		htod32a(dpv,value->a.count);
		for (i=0;i<value->a.count;i++)
			htod64a(&dpv[sizeof(uint32_t)+i*sizeof(int64_t)],value->a.v[i].i64);
		break;
	/* XXX: other int types are unimplemented */
	case PTP_DTC_STR: {
		dpv=ptp_get_packed_stringcopy(params, value->str, &size);
		break;
	}
	}
	*dpvptr=dpv;
	return size;
}

#define MAX_MTP_PROPS 127
static inline uint32_t
ptp_pack_OPL (PTPParams *params, MTPProperties *props, int nrofprops, unsigned char** opldataptr)
{
	unsigned char* opldata;
	MTPProperties *propitr;
	unsigned char *packedprops[MAX_MTP_PROPS];
	uint32_t packedpropslens[MAX_MTP_PROPS];
	uint32_t packedobjecthandles[MAX_MTP_PROPS];
	uint16_t packedpropsids[MAX_MTP_PROPS];
	uint16_t packedpropstypes[MAX_MTP_PROPS];
	uint32_t totalsize = 0;
	uint32_t bufp = 0;
	uint32_t noitems = 0;
	uint32_t i;

	totalsize = sizeof(uint32_t); /* 4 bytes to store the number of elements */
	propitr = props;
	while (nrofprops-- && noitems < MAX_MTP_PROPS) {
		/* Object Handle */
		packedobjecthandles[noitems]=propitr->ObjectHandle;
		totalsize += sizeof(uint32_t); /* Object ID */
		/* Metadata type */
		packedpropsids[noitems]=propitr->property;
		totalsize += sizeof(uint16_t);
		/* Data type */
		packedpropstypes[noitems]= propitr->datatype;
		totalsize += sizeof(uint16_t);
		/* Add each property to be sent. */
	        packedpropslens[noitems] = ptp_pack_DPV (params, &propitr->propval, &packedprops[noitems], propitr->datatype);
		totalsize += packedpropslens[noitems];
		noitems ++;
		propitr ++;
	}

	/* Allocate memory for the packed property list */
	opldata = malloc(totalsize);

	htod32a(&opldata[bufp],noitems);
	bufp += 4;

	/* Copy into a nice packed list */
	for (i = 0; i < noitems; i++) {
		/* Object ID */
		htod32a(&opldata[bufp],packedobjecthandles[i]);
		bufp += sizeof(uint32_t);
		htod16a(&opldata[bufp],packedpropsids[i]);
		bufp += sizeof(uint16_t);
		htod16a(&opldata[bufp],packedpropstypes[i]);
		bufp += sizeof(uint16_t);
		/* The copy the actual property */
		memcpy(&opldata[bufp], packedprops[i], packedpropslens[i]);
		bufp += packedpropslens[i];
		free(packedprops[i]);
	}
	*opldataptr = opldata;
	return totalsize;
}

static int
_compare_func(const void* x, const void *y) {
	const MTPProperties *px = x;
	const MTPProperties *py = y;

	return px->ObjectHandle - py->ObjectHandle;
}

static inline int
ptp_unpack_OPL (PTPParams *params, unsigned char* data, MTPProperties **pprops, unsigned int len)
{
	uint32_t prop_count;
	MTPProperties *props = NULL;
	unsigned int offset = 0, i;

	if (len < sizeof(uint32_t)) {
		ptp_debug (params ,"must have at least 4 bytes data, not %d", len);
		return 0;
	}

	prop_count = dtoh32a(data);
	*pprops = NULL;
	if (prop_count == 0)
		return 0;

	if (prop_count >= INT_MAX/sizeof(MTPProperties)) {
		ptp_debug (params ,"prop_count %d is too large", prop_count);
		return 0;
	}
	ptp_debug (params ,"Unpacking MTP OPL, size %d (prop_count %d)", len, prop_count);

	data += sizeof(uint32_t);
	len -= sizeof(uint32_t);
	props = malloc(prop_count * sizeof(MTPProperties));
	if (!props) return 0;
	for (i = 0; i < prop_count; i++) {
		if (len <= (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t))) {
			ptp_debug (params ,"short MTP Object Property List at property %d (of %d)", i, prop_count);
			ptp_debug (params ,"device probably needs DEVICE_FLAG_BROKEN_MTPGETOBJPROPLIST_ALL");
			ptp_debug (params ,"or even DEVICE_FLAG_BROKEN_MTPGETOBJPROPLIST", i);
			qsort (props, i, sizeof(MTPProperties),_compare_func);
			*pprops = props;
			return i;
		}


		props[i].ObjectHandle = dtoh32a(data);
		data += sizeof(uint32_t);
		len -= sizeof(uint32_t);

		props[i].property = dtoh16a(data);
		data += sizeof(uint16_t);
		len -= sizeof(uint16_t);

		props[i].datatype = dtoh16a(data);
		data += sizeof(uint16_t);
		len -= sizeof(uint16_t);

		offset = 0;
		if (!ptp_unpack_DPV(params, data, &offset, len, &props[i].propval, props[i].datatype)) {
			ptp_debug (params ,"unpacking DPV of property %d encountered insufficient buffer. attack?", i);
			qsort (props, i, sizeof(MTPProperties),_compare_func);
			*pprops = props;
			return i;
		}
		data += offset;
		len -= offset;
	}
	qsort (props, prop_count, sizeof(MTPProperties),_compare_func);
	*pprops = props;
	return prop_count;
}

/*
    PTP USB Event container unpack
    Copyright (c) 2003 Nikolai Kopanygin
*/

#define PTP_ec_Length		0
#define PTP_ec_Type		4
#define PTP_ec_Code		6
#define PTP_ec_TransId		8
#define PTP_ec_Param1		12
#define PTP_ec_Param2		16
#define PTP_ec_Param3		20

static inline void
ptp_unpack_EC (PTPParams *params, unsigned char* data, PTPContainer *ec, unsigned int len)
{
	unsigned int	length;
	int	type;

	if (data==NULL)
		return;
	memset(ec,0,sizeof(*ec));

	length=dtoh32a(&data[PTP_ec_Length]);
	if (length > len) {
		ptp_debug (params, "length %d in container, but data only %d bytes?!", length, len);
		return;
	}
	type = dtoh16a(&data[PTP_ec_Type]);

	ec->Code=dtoh16a(&data[PTP_ec_Code]);
	ec->Transaction_ID=dtoh32a(&data[PTP_ec_TransId]);

	if (type!=PTP_USB_CONTAINER_EVENT) {
		ptp_debug (params, "Unknown canon event type %d (code=%x,tid=%x), please report!",type,ec->Code,ec->Transaction_ID);
		return;
	}
	if (length>=(PTP_ec_Param1+4)) {
		ec->Param1=dtoh32a(&data[PTP_ec_Param1]);
		ec->Nparam=1;
	}
	if (length>=(PTP_ec_Param2+4)) {
		ec->Param2=dtoh32a(&data[PTP_ec_Param2]);
		ec->Nparam=2;
	}
	if (length>=(PTP_ec_Param3+4)) {
		ec->Param3=dtoh32a(&data[PTP_ec_Param3]);
		ec->Nparam=3;
	}
}

/*
    PTP Canon Folder Entry unpack
    Copyright (c) 2003 Nikolai Kopanygin
*/
#define PTP_cfe_ObjectHandle		0
#define PTP_cfe_ObjectFormatCode	4
#define PTP_cfe_Flags			6
#define PTP_cfe_ObjectSize		7
#define PTP_cfe_Time			11
#define PTP_cfe_Filename		15

static inline void
ptp_unpack_Canon_FE (PTPParams *params, unsigned char* data, PTPCANONFolderEntry *fe)
{
	int i;
	if (data==NULL)
		return;
	fe->ObjectHandle=dtoh32a(&data[PTP_cfe_ObjectHandle]);
	fe->ObjectFormatCode=dtoh16a(&data[PTP_cfe_ObjectFormatCode]);
	fe->Flags=dtoh8a(&data[PTP_cfe_Flags]);
	fe->ObjectSize=dtoh32a((unsigned char*)&data[PTP_cfe_ObjectSize]);
	fe->Time=(time_t)dtoh32a(&data[PTP_cfe_Time]);
	for (i=0; i<PTP_CANON_FilenameBufferLen; i++)
		fe->Filename[i]=(char)dtoh8a(&data[PTP_cfe_Filename+i]);
}

/*
    PTP Canon EOS Folder Entry unpack
0: 00 00 08 a0     objectid
4: 01 00 02 00     storageid
8: 01 30 00 00     ofc
12: 01 00
14: 00 00
16: 11 00 00 00
20: 00 00 00 00
24: 00 00 00 80
28: 00 00 08 a0
32: 4d 49 53 43-00 00 00 00 00 00 00 00     name
00 00 00 00
84 bc 74 46     objectime


(normal PTP GetObjectInfo)
ObjectInfo for 'IMG_0199.JPG':
  Object ID: 0x92740c72
  StorageID: 0x00020001
  ObjectFormat: 0x3801
  ProtectionStatus: 0x0000
  ObjectCompressedSize: 2217241
  ThumbFormat: 0x3808
  ThumbCompressedSize: 5122
  ThumbPixWidth: 160
  ThumbPixHeight: 120
  ImagePixWidth: 4000
  ImagePixHeight: 3000
  ImageBitDepth: 24
  ParentObject: 0x92740000
  AssociationType: 0x0000
  AssociationDesc: 0x00000000
  SequenceNumber: 0x00000000
  ModificationDate: 0x4d985ff0
  CaptureDate: 0x4d985ff0

0010  38 00 00 00  Size of this entry
0014  72 0c 74 92  OID
0018  01 00 02 00  StorageID
001c  01 38 00 00  OFC
0020  00 00 00 00 ??
0024  21 00 00 00  flags (4 bytes? 1 byte?)
0028  19 d5 21 00  Size
002c  00 00 74 92  ?
0030  70 0c 74 92  OID
0034  49 4d 47 5f-30 31 39 39 2e 4a 50 47  IMG_0199.JPG
0040  00 00 00 00
0044  10 7c 98 4d Time


*/
#define PTP_cefe_ObjectHandle		0
#define PTP_cefe_StorageID		4
#define PTP_cefe_ObjectFormatCode	8
#define PTP_cefe_Flags			16
#define PTP_cefe_ObjectSize		20
#define PTP_cefe_Filename		32
#define PTP_cefe_Time			48

static inline void
ptp_unpack_Canon_EOS_FE (PTPParams *params, unsigned char* data, unsigned int size, PTPCANONFolderEntry *fe)
{
	int i;

	if (size < PTP_cefe_Time + 4) return;

	fe->ObjectHandle=dtoh32a(&data[PTP_cefe_ObjectHandle]);
	fe->ObjectFormatCode=dtoh16a(&data[PTP_cefe_ObjectFormatCode]);
	fe->Flags=dtoh8a(&data[PTP_cefe_Flags]);
	fe->ObjectSize=dtoh32a((unsigned char*)&data[PTP_cefe_ObjectSize]);
	fe->Time=(time_t)dtoh32a(&data[PTP_cefe_Time]);
	for (i=0; i<PTP_CANON_FilenameBufferLen; i++)
		fe->Filename[i]=(char)data[PTP_cefe_Filename+i];
	fe->Filename[PTP_CANON_FilenameBufferLen-1] = 0;
}


static inline uint16_t
ptp_unpack_EOS_ImageFormat (PTPParams* params, unsigned char** data )
{
	/*
	  EOS ImageFormat entries (of at least the 5DM2 and the 400D) look like this:
		uint32: number of entries / generated files (1 or 2)
		uint32: size of this entry in bytes (most likely allways 0x10)
		uint32: image type (1 == JPG, 6 == RAW)
		uint32: image size (0 == Large, 1 == Medium, 2 == Small, 0xe == S1, 0xf == S2, 0x10 == S3)
		uint32: image compression (2 == Standard/JPG, 3 == Fine/JPG, 4 == Lossles/RAW)
	  If the number of entries is 2 the last 4 uint32 repeat.

	  example:
		0: 0x       1
		1: 0x      10
		2: 0x       6
		3: 0x       1
		4: 0x       4

	  The idea is to simply 'condense' these values to just one uint16 to be able to conveniently
	  use the available enumeration facilities (look-up table). The image size and compression
	  values used to fully describe the image format, but at least since EOS M50 (with cRAW)
	  it is no longer true - we need to store RAW flag (8).
	  Hence we generate a uint16 with the four nibles set as follows:

	  entry 1 size | entry 1 compression & RAW flag | entry 2 size | entry 2 compression & RAW flag.

	  The above example would result in the value 0x1400.

	  The EOS 5D Mark III (and possibly other high-end EOS as well) added the extra fancy S1, S2
	  and S3 JPEG options. S1 replaces the old Small. -1 the S1/S2/S3 to prevent the 0x10 overflow.
	  */

	const unsigned char* d = *data;
	uint32_t n = dtoh32a( d );
	uint32_t l, t1, s1, c1, t2 = 0, s2 = 0, c2 = 0;

	if (n != 1 && n !=2) {
		ptp_debug (params, "parsing EOS ImageFormat property failed (n != 1 && n != 2: %d)", n);
		return 0;
	}

	l = dtoh32a( d+=4 );
	if (l != 0x10) {
		ptp_debug (params, "parsing EOS ImageFormat property failed (l != 0x10: 0x%x)", l);
		return 0;
	}

	t1 = dtoh32a( d+=4 );
	s1 = dtoh32a( d+=4 );
	c1 = dtoh32a( d+=4 );

	if (n == 2) {
		l = dtoh32a( d+=4 );
		if (l != 0x10) {
			ptp_debug (params, "parsing EOS ImageFormat property failed (l != 0x10: 0x%x)", l);
			return 0;
		}
		t2 = dtoh32a( d+=4 );
		s2 = dtoh32a( d+=4 );
		c2 = dtoh32a( d+=4 );
	}

	*data = (unsigned char*) d+4;

	/* deal with S1/S2/S3 JPEG sizes, see above. */
	if( s1 >= 0xe )
		s1--;
	if( s2 >= 0xe )
		s2--;

	/* encode RAW flag */
	c1 |= (t1 == 6) ? 8 : 0;
	c2 |= (t2 == 6) ? 8 : 0;

	return ((s1 & 0xF) << 12) | ((c1 & 0xF) << 8) | ((s2 & 0xF) << 4) | ((c2 & 0xF) << 0);
}

static inline uint32_t
ptp_pack_EOS_ImageFormat (PTPParams* params, unsigned char* data, uint16_t value)
{
	uint32_t n = (value & 0xFF) ? 2 : 1;
	uint32_t s = 4 + 0x10 * n;

	if( !data )
		return s;

#define PACK_5DM3_SMALL_JPEG_SIZE( X ) (X) >= 0xd ? (X)+1 : (X)

	htod32a(data+=0, n);
	htod32a(data+=4, 0x10);
	htod32a(data+=4, (((value >> 8) & 0xF) >> 3) ? 6 : 1);
	htod32a(data+=4, PACK_5DM3_SMALL_JPEG_SIZE((value >> 12) & 0xF));
	htod32a(data+=4, ((value >> 8) & 0xF) & ~8);

	if (n==2) {
		htod32a(data+=4, 0x10);
		htod32a(data+=4, (((value >> 0) & 0xF) >> 3) ? 6 : 1);
		htod32a(data+=4, PACK_5DM3_SMALL_JPEG_SIZE((value >> 4) & 0xF));
		htod32a(data+=4, ((value >> 0) & 0xF) & ~8);
	}

#undef PACK_5DM3_SMALL_JPEG_SIZE

	return s;
}

/* 00: 32 bit size
 * 04: 16 bit subsize
 * 08: 16 bit version (?)
 * 0c: 16 bit focus_points_in_struct
 * 10: 16 bit focus_points_in_use
 * 14: variable arrays:
 * 	16 bit sizex, 16 bit sizey
 * 	16 bit othersizex, 16 bit othersizey
 * 	16 bit array height[focus_points_in_struct]
 * 	16 bit array width[focus_points_in_struct]
 * 	16 bit array offsetheight[focus_points_in_struct] middle is 0
 * 	16 bit array offsetwidth[focus_points_in_struct] middle is ?
 * bitfield of selected focus points, starting with 0 [size focus_points_in_struct in bits]
 * unknown stuff , likely which are active
 * 16 bit 0xffff
 *
 * size=NxN,size2=NxN,points={NxNxNxN,NxNxNxN,...},selected={0,1,2}
 */
static inline char*
ptp_unpack_EOS_FocusInfoEx (PTPParams* params, unsigned char** data, uint32_t datasize )
{
	uint32_t size 			= dtoh32a( *data );
	uint32_t halfsize		= dtoh16a( (*data) + 4);
	uint32_t version		= dtoh16a( (*data) + 6);
	uint32_t focus_points_in_struct	= dtoh16a( (*data) + 8);
	uint32_t focus_points_in_use	= dtoh16a( (*data) + 10);
	uint32_t sizeX			= dtoh16a( (*data) + 12);
	uint32_t sizeY			= dtoh16a( (*data) + 14);
	uint32_t size2X			= dtoh16a( (*data) + 16);
	uint32_t size2Y			= dtoh16a( (*data) + 18);
	uint32_t i;
	uint32_t maxlen;
	char	*str, *p;

	if ((size >= datasize) || (size < 20))
		return strdup("bad size 1");
	/* If data is zero-filled, then it is just a placeholder, so nothing
	   useful, but also not an error */
	if (!focus_points_in_struct || !focus_points_in_use) {
		ptp_debug(params, "skipped FocusInfoEx data (zero filled)");
		return strdup("no focus points returned by camera");
	}

	/* every focuspoint gets 4 (16 bit number possible "-" sign and a x) and a ,*/
	/* initial things around lets say 100 chars at most.
	 * FIXME: check selected when we decode it
	 */
	if (size < focus_points_in_struct*8) {
		ptp_error(params, "focus_points_in_struct %d is too large vs size %d", focus_points_in_struct, size);
		return strdup("bad size 2");
	}
	if (focus_points_in_use > focus_points_in_struct) {
		ptp_error(params, "focus_points_in_use %d is larger than focus_points_in_struct %d", focus_points_in_use, focus_points_in_struct);
		return strdup("bad size 3");
	}

	maxlen = focus_points_in_use*32 + 100 + (size - focus_points_in_struct*8)*2;
	if (halfsize != size-4) {
		ptp_error(params, "halfsize %d is not expected %d", halfsize, size-4);
		return strdup("bad size 4");
	}
	if (20 + focus_points_in_struct*8 + (focus_points_in_struct+7)/8 > size) {
		ptp_error(params, "size %d is too large for fp in struct %d", focus_points_in_struct*8 + 20 + (focus_points_in_struct+7)/8, size);
		return strdup("bad size 5");
	}
#if 0
	ptp_debug(params,"d1d3 content:");
	for (i=0;i<size;i+=2)
		ptp_debug(params,"%d: %02x %02x", i, (*data)[i], (*data)[i+1]);
#endif
	ptp_debug(params,"d1d3 version %d", version);
	ptp_debug(params,"d1d3 size %d", size);
	ptp_debug(params,"d1d3 focus points in struct %d, in use %d", focus_points_in_struct, focus_points_in_use);

	str = (char*)malloc( maxlen );
	if (!str)
		return NULL;
	p = str;

	p += sprintf(p,"eosversion=%u,size=%ux%u,size2=%ux%u,points={", version, sizeX, sizeY, size2X, size2Y);
	for (i=0;i<focus_points_in_use;i++) {
		int16_t x = dtoh16a((*data) + focus_points_in_struct*4 + 20 + 2*i);
		int16_t y = dtoh16a((*data) + focus_points_in_struct*6 + 20 + 2*i);
		int16_t w = dtoh16a((*data) + focus_points_in_struct*2 + 20 + 2*i);
		int16_t h = dtoh16a((*data) + focus_points_in_struct*0 + 20 + 2*i);

		p += sprintf(p,"{%d,%d,%d,%d}",x,y,w,h);

		if (i<focus_points_in_use-1)
			p += sprintf(p,",");
	}
	p += sprintf(p,"},select={");
	for (i=0;i<focus_points_in_use;i++) {
		if ((1<<(i%8)) & ((*data)[focus_points_in_struct*8+20+i/8]))
			p+=sprintf(p,"%u,", i);
	}

	p += sprintf(p,"},unknown={");
	for (i=focus_points_in_struct*8+(focus_points_in_struct+7)/8+20;i<size;i++) {
		if ((p-str) > maxlen - 4)
			break;
		p+=sprintf(p,"%02x", (*data)[i]);
	}
	p += sprintf(p,"}");
	return str;
}


static inline char*
ptp_unpack_EOS_CustomFuncEx (PTPParams* params, unsigned char** data )
{
	uint32_t s = dtoh32a( *data );
	uint32_t n = s/4, i;
	char	*str, *p;

	if (s > 1024) {
		ptp_debug (params, "customfuncex data is larger than 1k / %d... unexpected?", s);
		return strdup("bad length");
	}
	str = (char*)malloc( s*2+s/4+1 ); /* n is size in uint32, maximum %x len is 8 chars and \0*/
	if (!str)
		return strdup("malloc failed");

	p = str;
	for (i=0; i < n; ++i)
		p += sprintf(p, "%x,", dtoh32a( *data + 4*i ));
	return str;
}

static inline uint32_t
ptp_pack_EOS_CustomFuncEx (PTPParams* params, unsigned char* data, char* str)
{
	uint32_t s = strtoul(str, NULL, 16);
	uint32_t n = s/4, i, v;

	if (!data)
		return s;

	for (i=0; i<n; i++)
	{
		v = strtoul(str, &str, 16);
		str++; /* skip the ',' delimiter */
		htod32a(data + i*4, v);
	}

	return s;
}

/*
    PTP EOS Changes Entry unpack
*/
#define PTP_ece_Size		0
#define PTP_ece_Type		4

#define PTP_ece_Prop_Subtype	8	/* only for properties */
#define PTP_ece_Prop_Val_Data	0xc	/* only for properties */
#define PTP_ece_Prop_Desc_Type	0xc	/* only for property descs */
#define PTP_ece_Prop_Desc_Count	0x10	/* only for property descs */
#define PTP_ece_Prop_Desc_Data	0x14	/* only for property descs */

/* for PTP_EC_CANON_EOS_RequestObjectTransfer */
#define PTP_ece_OI_ObjectID	8
#define PTP_ece_OI_OFC		0x0c
#define PTP_ece_OI_Size		0x14
#define PTP_ece_OI_Name		0x1c

/* for PTP_EC_CANON_EOS_ObjectAddedEx */
#define PTP_ece_OA_ObjectID	8
#define PTP_ece_OA_StorageID	0x0c
#define PTP_ece_OA_OFC		0x10
#define PTP_ece_OA_Size		0x1c
#define PTP_ece_OA_Parent	0x20
#define PTP_ece_OA_Name		0x28

#define PTP_ece2_OA_ObjectID	8	/* OK */
#define PTP_ece2_OA_StorageID	0x0c	/* OK */
#define PTP_ece2_OA_OFC		0x10	/* OK */
#define PTP_ece2_OA_Size	0x1c	/* OK, might be 64 bit now? */
#define PTP_ece2_OA_Parent	0x24
#define PTP_ece2_OA_2ndOID	0x28
#define PTP_ece2_OA_Name	0x2c	/* OK */

/* for PTP_EC_CANON_EOS_ObjectAddedNew */
#define PTP_ece_OAN_OFC		0x0c
#define PTP_ece_OAN_Size	0x14

static PTPDevicePropDesc*
_lookup_or_allocate_canon_prop(PTPParams *params, uint16_t proptype)
{
	unsigned int j;

	for (j=0;j<params->nrofcanon_props;j++)
		if (params->canon_props[j].proptype == proptype)
			break;
	if (j<params->nrofcanon_props)
		return &params->canon_props[j].dpd;

	if (j)
		params->canon_props = realloc(params->canon_props, sizeof(params->canon_props[0])*(j+1));
	else
		params->canon_props = malloc(sizeof(params->canon_props[0]));
	params->canon_props[j].proptype = proptype;
	params->canon_props[j].size = 0;
	params->canon_props[j].data = NULL;
	memset (&params->canon_props[j].dpd,0,sizeof(params->canon_props[j].dpd));
	params->canon_props[j].dpd.GetSet = 1;
	params->canon_props[j].dpd.FormFlag = PTP_DPFF_None;
	params->nrofcanon_props = j+1;
	return &params->canon_props[j].dpd;
}


static inline int
ptp_unpack_CANON_changes (PTPParams *params, unsigned char* data, unsigned int datasize, PTPCanon_changes_entry **pce)
{
	int	i = 0, entries = 0;
	unsigned char	*curdata = data;
	PTPCanon_changes_entry *ce;

	if (data==NULL)
		return 0;
	while (curdata - data + 8 < datasize) {
		uint32_t	size = dtoh32a(&curdata[PTP_ece_Size]);
		uint32_t	type = dtoh32a(&curdata[PTP_ece_Type]);

		if (size > datasize) {
			ptp_debug (params, "size %d is larger than datasize %d", size, datasize);
			break;
		}
		if (size < 8) {
			ptp_debug (params, "size %d is smaller than 8.", size);
			break;
		}
		if ((size == 8) && (type == 0))
			break;
		if ((curdata - data) + size >= datasize) {
			ptp_debug (params, "canon eos event decoder ran over supplied data, skipping entries");
			break;
		}
		if (type == PTP_EC_CANON_EOS_OLCInfoChanged) {
			unsigned int j;

			entries++;
			if (size >= 12+2) {
				for (j=0;j<31;j++)
					if (dtoh16a(curdata+12) & (1<<j))
						entries++;
			}
		}
		curdata += size;
		entries++;
	}
	ce = malloc (sizeof(PTPCanon_changes_entry)*(entries+1));
	if (!ce) return 0;

	curdata = data;
	while (curdata - data  + 8 < datasize) {
		uint32_t	size = dtoh32a(&curdata[PTP_ece_Size]);
		uint32_t	type = dtoh32a(&curdata[PTP_ece_Type]);

		if (size > datasize) {
			ptp_debug (params, "size %d is larger than datasize %d", size, datasize);
			break;
		}
		if (size < 8) {
			ptp_debug (params, "size %d is smaller than 8", size);
			break;
		}

		if ((size == 8) && (type == 0))
			break;

		if ((curdata - data) + size >= datasize) {
			ptp_debug (params, "canon eos event decoder ran over supplied data, skipping entries");
			break;
		}

		ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
		ce[i].u.info = NULL;
		switch (type) {
		case PTP_EC_CANON_EOS_ObjectContentChanged:
			if (size < PTP_ece_OA_ObjectID+1) {
				ptp_debug (params, "size %d is smaller than %d", size, PTP_ece_OA_ObjectID+1);
				break;
			}
			ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_OBJECTCONTENT_CHANGE;
			ce[i].u.object.oid    		= dtoh32a(&curdata[PTP_ece_OA_ObjectID]);
			break;
		case PTP_EC_CANON_EOS_ObjectInfoChangedEx:
		case PTP_EC_CANON_EOS_ObjectAddedEx:
			if (size < PTP_ece_OA_Name+1) {
				ptp_debug (params, "size %d is smaller than %d", size, PTP_ece_OA_Name+1);
				break;
			}
			ce[i].type = ((type == PTP_EC_CANON_EOS_ObjectAddedEx) ? PTP_CANON_EOS_CHANGES_TYPE_OBJECTINFO : PTP_CANON_EOS_CHANGES_TYPE_OBJECTINFO_CHANGE);
			ce[i].u.object.oid    		= dtoh32a(&curdata[PTP_ece_OA_ObjectID]);
			ce[i].u.object.oi.StorageID	= dtoh32a(&curdata[PTP_ece_OA_StorageID]);
			ce[i].u.object.oi.ParentObject	= dtoh32a(&curdata[PTP_ece_OA_Parent]);
			ce[i].u.object.oi.ObjectFormat 	= dtoh16a(&curdata[PTP_ece_OA_OFC]);
			ce[i].u.object.oi.ObjectCompressedSize= dtoh32a(&curdata[PTP_ece_OA_Size]);
			ce[i].u.object.oi.Filename 	= strdup(((char*)&curdata[PTP_ece_OA_Name]));
			if (type == PTP_EC_CANON_EOS_ObjectAddedEx) {
				ptp_debug (params, "event %d: objectinfo added oid %08lx, parent %08lx, ofc %04x, size %d, filename %s", i, ce[i].u.object.oid, ce[i].u.object.oi.ParentObject, ce[i].u.object.oi.ObjectFormat, ce[i].u.object.oi.ObjectCompressedSize, ce[i].u.object.oi.Filename);
			} else {
				ptp_debug (params, "event %d: objectinfo changed oid %08lx, parent %08lx, ofc %04x, size %d, filename %s", i, ce[i].u.object.oid, ce[i].u.object.oi.ParentObject, ce[i].u.object.oi.ObjectFormat, ce[i].u.object.oi.ObjectCompressedSize, ce[i].u.object.oi.Filename);
			}
			break;
		case PTP_EC_CANON_EOS_ObjectAddedEx64:	/* FIXME: review if the data used is correct */
			if (size < PTP_ece2_OA_Name+1) {
				ptp_debug (params, "size %d is smaller than %d", size, PTP_ece2_OA_Name+1);
				break;
			}
			ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_OBJECTINFO;
			ce[i].u.object.oid    		= dtoh32a(&curdata[PTP_ece2_OA_ObjectID]);
			ce[i].u.object.oi.StorageID	= dtoh32a(&curdata[PTP_ece2_OA_StorageID]);
			ce[i].u.object.oi.ParentObject	= dtoh32a(&curdata[PTP_ece2_OA_Parent]);
			ce[i].u.object.oi.ObjectFormat 	= dtoh16a(&curdata[PTP_ece2_OA_OFC]);
			ce[i].u.object.oi.ObjectCompressedSize= dtoh32a(&curdata[PTP_ece2_OA_Size]);	/* FIXME: might be 64bit now */
			ce[i].u.object.oi.Filename 	= strdup(((char*)&curdata[PTP_ece2_OA_Name]));
			ptp_debug (params, "event %d: objectinfo added oid %08lx, parent %08lx, ofc %04x, size %d, filename %s", i, ce[i].u.object.oid, ce[i].u.object.oi.ParentObject, ce[i].u.object.oi.ObjectFormat, ce[i].u.object.oi.ObjectCompressedSize, ce[i].u.object.oi.Filename);
			break;
		case PTP_EC_CANON_EOS_RequestObjectTransfer:
		case PTP_EC_CANON_EOS_RequestObjectTransfer64:
			if (size < PTP_ece_OI_Name+1) {
				ptp_debug (params, "size %d is smaller than %d", size, PTP_ece_OI_Name+1);
				break;
			}
			ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_OBJECTTRANSFER;
			ce[i].u.object.oid    		= dtoh32a(&curdata[PTP_ece_OI_ObjectID]);
			ce[i].u.object.oi.StorageID 	= 0; /* use as marker */
			ce[i].u.object.oi.ObjectFormat 	= dtoh16a(&curdata[PTP_ece_OI_OFC]);
			ce[i].u.object.oi.ParentObject	= 0; /* check, but use as marker */
			ce[i].u.object.oi.ObjectCompressedSize = dtoh32a(&curdata[PTP_ece_OI_Size]);
			ce[i].u.object.oi.Filename 	= strdup(((char*)&curdata[PTP_ece_OI_Name]));

			ptp_debug (params, "event %d: request object transfer oid %08lx, ofc %04x, size %d, filename %p", i, ce[i].u.object.oid, ce[i].u.object.oi.ObjectFormat, ce[i].u.object.oi.ObjectCompressedSize, ce[i].u.object.oi.Filename);
			break;
		case PTP_EC_CANON_EOS_AvailListChanged: {	/* property desc */
			uint32_t	proptype = dtoh32a(&curdata[PTP_ece_Prop_Subtype]);
			uint32_t	propxtype = dtoh32a(&curdata[PTP_ece_Prop_Desc_Type]);
			uint32_t	propxcnt = dtoh32a(&curdata[PTP_ece_Prop_Desc_Count]);
			unsigned char	*xdata = &curdata[PTP_ece_Prop_Desc_Data];
			unsigned int	j;
			PTPDevicePropDesc	*dpd;

			if (size < PTP_ece_Prop_Desc_Data) {
				ptp_debug (params, "size %d is smaller than %d", size, PTP_ece_Prop_Desc_Data);
				break;
			}

			ptp_debug (params, "event %d: EOS prop %04x (%s) desc record, datasize %d, propxtype %d", i, proptype, ptp_get_property_description (params, proptype), size-PTP_ece_Prop_Desc_Data, propxtype);
			for (j=0;j<params->nrofcanon_props;j++)
				if (params->canon_props[j].proptype == proptype)
					break;
			if (j==params->nrofcanon_props) {
				ptp_debug (params, "event %d: propdesc %x, default value not found.", i, proptype);
				break;
			}
			dpd = &params->canon_props[j].dpd;
			/* 1 - uint16 ?
			 * 3 - uint16
			 * 7 - string?
			 */
			if (propxtype != 3) {
				ptp_debug (params, "event %d: propxtype is %x for %04x, unhandled, size %d", i, propxtype, proptype, size);
				for (j=0;j<size-PTP_ece_Prop_Desc_Data;j++)
					ptp_debug (params, "    %d: %02x", j, xdata[j]);
				break;
			}
			if (! propxcnt)
				break;
			if (propxcnt >= 2<<16) /* buggy or exploit */
				break;

			ptp_debug (params, "event %d: propxtype is %x, prop is 0x%04x, data type is 0x%04x, propxcnt is %d.",
				   i, propxtype, proptype, dpd->DataType, propxcnt);
			dpd->FormFlag = PTP_DPFF_Enumeration;
			dpd->FORM.Enum.NumberOfValues = propxcnt;
			free (dpd->FORM.Enum.SupportedValue);
			dpd->FORM.Enum.SupportedValue = malloc (sizeof (PTPPropertyValue)*propxcnt);

			switch (proptype) {
			case PTP_DPC_CANON_EOS_ImageFormat:
			case PTP_DPC_CANON_EOS_ImageFormatCF:
			case PTP_DPC_CANON_EOS_ImageFormatSD:
			case PTP_DPC_CANON_EOS_ImageFormatExtHD:
				/* special handling of ImageFormat properties */
				for (j=0;j<propxcnt;j++) {
					dpd->FORM.Enum.SupportedValue[j].u16 =
							ptp_unpack_EOS_ImageFormat( params, &xdata );
					ptp_debug (params, "event %d: suppval[%d] of %x is 0x%x.", i, j, proptype, dpd->FORM.Enum.SupportedValue[j].u16);
				}
				break;
			default:
				/* 'normal' enumerated types */
				switch (dpd->DataType) {
#define XX( TYPE, CONV )\
					if (sizeof(dpd->FORM.Enum.SupportedValue[j].TYPE)*propxcnt + PTP_ece_Prop_Desc_Data > size) {	\
						ptp_debug (params, "size %lu does not match needed %u", sizeof(dpd->FORM.Enum.SupportedValue[j].TYPE)*propxcnt + PTP_ece_Prop_Desc_Data, size);	\
						break;							\
					}								\
					for (j=0;j<propxcnt;j++) { 					\
						dpd->FORM.Enum.SupportedValue[j].TYPE = CONV(xdata); 	\
						ptp_debug (params, "event %u: suppval[%u] of %x is 0x%x.", i, j, proptype, CONV(xdata)); \
						xdata += 4; /* might only be for propxtype 3 */ \
					} \
					break;

				case PTP_DTC_INT16:	XX( i16, dtoh16a );
				case PTP_DTC_UINT16:	XX( u16, dtoh16a );
				case PTP_DTC_UINT32:	XX( u32, dtoh32a );
				case PTP_DTC_INT32:	XX( i32, dtoh32a );
				case PTP_DTC_UINT8:	XX( u8,  dtoh8a );
				case PTP_DTC_INT8:	XX( i8,  dtoh8a );
#undef XX
				default:
					free (dpd->FORM.Enum.SupportedValue);
					dpd->FORM.Enum.SupportedValue = NULL;
					dpd->FORM.Enum.NumberOfValues = 0;
					ptp_debug (params ,"event %d: data type 0x%04x of %x unhandled, size %d, raw values:", i, dpd->DataType, proptype, dtoh32a(xdata), size);
					for (j=0;j<(size-PTP_ece_Prop_Desc_Data)/4;j++, xdata+=4) /* 4 is good for propxtype 3 */
						ptp_debug (params, "    %3d: 0x%8x", j, dtoh32a(xdata));
					break;
				}
			}
			break;
		}
		case PTP_EC_CANON_EOS_PropValueChanged:
			if (size >= 0xc) {	/* property info */
				unsigned int j;
				uint32_t	proptype = dtoh32a(&curdata[PTP_ece_Prop_Subtype]);
				unsigned char	*xdata = &curdata[PTP_ece_Prop_Val_Data];
				PTPDevicePropDesc	*dpd;

				if (size < PTP_ece_Prop_Val_Data) {
					ptp_debug (params, "size %d is smaller than %d", size, PTP_ece_Prop_Val_Data);
					break;
				}
				ptp_debug (params, "event %d: EOS prop %04x (%s) info record, datasize is %d", i, proptype, ptp_get_property_description(params,proptype), size-PTP_ece_Prop_Val_Data);
				for (j=0;j<params->nrofcanon_props;j++)
					if (params->canon_props[j].proptype == proptype)
						break;
				if (j<params->nrofcanon_props) {
					if (	(params->canon_props[j].size != size) ||
						(memcmp(params->canon_props[j].data,xdata,size-PTP_ece_Prop_Val_Data))) {
						params->canon_props[j].data = realloc(params->canon_props[j].data,size-PTP_ece_Prop_Val_Data);
						params->canon_props[j].size = size;
						memcpy (params->canon_props[j].data,xdata,size-PTP_ece_Prop_Val_Data);
					}
				} else {
					if (j)
						params->canon_props = realloc(params->canon_props, sizeof(params->canon_props[0])*(j+1));
					else
						params->canon_props = malloc(sizeof(params->canon_props[0]));
					params->canon_props[j].proptype = proptype;
					params->canon_props[j].size = size;
					params->canon_props[j].data = malloc(size-PTP_ece_Prop_Val_Data);
					memcpy(params->canon_props[j].data, xdata, size-PTP_ece_Prop_Val_Data);
					memset (&params->canon_props[j].dpd,0,sizeof(params->canon_props[j].dpd));
					params->canon_props[j].dpd.GetSet = 1;
					params->canon_props[j].dpd.FormFlag = PTP_DPFF_None;
					params->nrofcanon_props = j+1;
				}
				dpd = &params->canon_props[j].dpd;

				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_PROPERTY;
				ce[i].u.propid = proptype;

				/* fix GetSet value */
				switch (proptype) {
#define XX(x) case PTP_DPC_CANON_##x:
					XX(EOS_FocusMode)
					XX(EOS_BatteryPower)
					XX(EOS_BatterySelect)
					XX(EOS_ModelID)
					XX(EOS_PTPExtensionVersion)
					XX(EOS_DPOFVersion)
					XX(EOS_AvailableShots)
					XX(EOS_CurrentStorage)
					XX(EOS_CurrentFolder)
					XX(EOS_MyMenu)
					XX(EOS_MyMenuList)
					XX(EOS_HDDirectoryStructure)
					XX(EOS_BatteryInfo)
					XX(EOS_AdapterInfo)
					XX(EOS_LensStatus)
					XX(EOS_CardExtension)
					XX(EOS_TempStatus)
					XX(EOS_ShutterCounter)
					XX(EOS_SerialNumber)
					XX(EOS_DepthOfFieldPreview)
					XX(EOS_EVFRecordStatus)
					XX(EOS_LvAfSystem)
					XX(EOS_FocusInfoEx)
					XX(EOS_DepthOfField)
					XX(EOS_Brightness)
					XX(EOS_EFComp)
					XX(EOS_LensName)
					XX(EOS_LensID)
					XX(EOS_FixedMovie)
#undef XX
						dpd->GetSet = PTP_DPGS_Get;
						break;
				}

				/* set DataType */
				switch (proptype) {
				case PTP_DPC_CANON_EOS_CameraTime:
				case PTP_DPC_CANON_EOS_UTCTime:
				case PTP_DPC_CANON_EOS_Summertime: /* basical the DST flag */
				case PTP_DPC_CANON_EOS_AvailableShots:
				case PTP_DPC_CANON_EOS_CaptureDestination:
				case PTP_DPC_CANON_EOS_WhiteBalanceXA:
				case PTP_DPC_CANON_EOS_WhiteBalanceXB:
				case PTP_DPC_CANON_EOS_CurrentStorage:
				case PTP_DPC_CANON_EOS_CurrentFolder:
				case PTP_DPC_CANON_EOS_ShutterCounter:
				case PTP_DPC_CANON_EOS_ModelID:
				case PTP_DPC_CANON_EOS_LensID:
				case PTP_DPC_CANON_EOS_StroboFiring:
				case PTP_DPC_CANON_EOS_AFSelectFocusArea:
				case PTP_DPC_CANON_EOS_ContinousAFMode:
				case PTP_DPC_CANON_EOS_MirrorUpSetting:
				case PTP_DPC_CANON_EOS_OLCInfoVersion:
				case PTP_DPC_CANON_EOS_PowerZoomPosition:
				case PTP_DPC_CANON_EOS_PowerZoomSpeed:
				case PTP_DPC_CANON_EOS_BuiltinStroboMode:
				case PTP_DPC_CANON_EOS_StroboETTL2Metering:
				case PTP_DPC_CANON_EOS_ColorTemperature:
				case PTP_DPC_CANON_EOS_FixedMovie:
					dpd->DataType = PTP_DTC_UINT32;
					break;
				/* enumeration for AEM is never provided, but is available to set */
				case PTP_DPC_CANON_EOS_AutoExposureMode:
					dpd->DataType = PTP_DTC_UINT16;
					dpd->FormFlag = PTP_DPFF_Enumeration;
					dpd->FORM.Enum.NumberOfValues = 0;
					break;
				case PTP_DPC_CANON_EOS_Aperture:
				case PTP_DPC_CANON_EOS_ShutterSpeed:
				case PTP_DPC_CANON_EOS_ISOSpeed:
				case PTP_DPC_CANON_EOS_FocusMode:
				case PTP_DPC_CANON_EOS_ColorSpace:
				case PTP_DPC_CANON_EOS_BatteryPower:
				case PTP_DPC_CANON_EOS_BatterySelect:
				case PTP_DPC_CANON_EOS_PTPExtensionVersion:
				case PTP_DPC_CANON_EOS_DriveMode:
				case PTP_DPC_CANON_EOS_AEB:
				case PTP_DPC_CANON_EOS_BracketMode:
				case PTP_DPC_CANON_EOS_QuickReviewTime:
				case PTP_DPC_CANON_EOS_EVFMode:
				case PTP_DPC_CANON_EOS_EVFOutputDevice:
				case PTP_DPC_CANON_EOS_AutoPowerOff:
				case PTP_DPC_CANON_EOS_EVFRecordStatus:
				case PTP_DPC_CANON_EOS_HighISOSettingNoiseReduction:
				case PTP_DPC_CANON_EOS_MultiAspect: /* actually a 32bit value, but lets try it for easyness */
					dpd->DataType = PTP_DTC_UINT16;
					break;
				case PTP_DPC_CANON_EOS_PictureStyle:
				case PTP_DPC_CANON_EOS_WhiteBalance:
				case PTP_DPC_CANON_EOS_MeteringMode:
				case PTP_DPC_CANON_EOS_ExpCompensation:
					dpd->DataType = PTP_DTC_UINT8;
					break;
				case PTP_DPC_CANON_EOS_Owner:
				case PTP_DPC_CANON_EOS_Artist:
				case PTP_DPC_CANON_EOS_Copyright:
				case PTP_DPC_CANON_EOS_SerialNumber:
				case PTP_DPC_CANON_EOS_LensName:
					dpd->DataType = PTP_DTC_STR;
					break;
				case PTP_DPC_CANON_EOS_WhiteBalanceAdjustA:
				case PTP_DPC_CANON_EOS_WhiteBalanceAdjustB:
					dpd->DataType = PTP_DTC_INT32;
					break;
				/* unknown props, listed from dump.... all 16 bit, but vals might be smaller */
				case PTP_DPC_CANON_EOS_DPOFVersion:
					dpd->DataType = PTP_DTC_UINT16;
					ptp_debug (params, "event %d: Unknown EOS property %04x, datasize is %d, using uint16", i ,proptype, size-PTP_ece_Prop_Val_Data);
					for (j=0;j<size-PTP_ece_Prop_Val_Data;j++)
						ptp_debug (params, "    %d: %02x", j, xdata[j]);
					break;
				case PTP_DPC_CANON_EOS_CustomFunc1:
				case PTP_DPC_CANON_EOS_CustomFunc2:
				case PTP_DPC_CANON_EOS_CustomFunc3:
				case PTP_DPC_CANON_EOS_CustomFunc4:
				case PTP_DPC_CANON_EOS_CustomFunc5:
				case PTP_DPC_CANON_EOS_CustomFunc6:
				case PTP_DPC_CANON_EOS_CustomFunc7:
				case PTP_DPC_CANON_EOS_CustomFunc8:
				case PTP_DPC_CANON_EOS_CustomFunc9:
				case PTP_DPC_CANON_EOS_CustomFunc10:
				case PTP_DPC_CANON_EOS_CustomFunc11:
					dpd->DataType = PTP_DTC_UINT8;
					ptp_debug (params, "event %d: Unknown EOS property %04x, datasize is %d, using uint8", i ,proptype, size-PTP_ece_Prop_Val_Data);
					for (j=0;j<size-PTP_ece_Prop_Val_Data;j++)
						ptp_debug (params, "    %d: %02x", j, xdata[j]);
					/* custom func entries look like this on the 400D: '5 0 0 0 ?' = 4 bytes size + 1 byte data */
					xdata += 4;
					break;
				/* yet unknown 32bit props */
				case PTP_DPC_CANON_EOS_WftStatus:
				case PTP_DPC_CANON_EOS_LensStatus:
				case PTP_DPC_CANON_EOS_CardExtension:
				case PTP_DPC_CANON_EOS_TempStatus:
				case PTP_DPC_CANON_EOS_PhotoStudioMode:
				case PTP_DPC_CANON_EOS_DepthOfFieldPreview:
				case PTP_DPC_CANON_EOS_EVFSharpness:
				case PTP_DPC_CANON_EOS_EVFWBMode:
				case PTP_DPC_CANON_EOS_EVFClickWBCoeffs:
				case PTP_DPC_CANON_EOS_EVFColorTemp:
				case PTP_DPC_CANON_EOS_ExposureSimMode:
				case PTP_DPC_CANON_EOS_LvAfSystem:
				case PTP_DPC_CANON_EOS_MovSize:
				case PTP_DPC_CANON_EOS_DepthOfField:
				case PTP_DPC_CANON_EOS_LvViewTypeSelect:
				case PTP_DPC_CANON_EOS_AloMode:
				case PTP_DPC_CANON_EOS_Brightness:
				case PTP_DPC_CANON_EOS_GPSLogCtrl:
				case PTP_DPC_CANON_EOS_GPSDeviceActive:
					dpd->DataType = PTP_DTC_UINT32;
					ptp_debug (params, "event %d: Unknown EOS property %04x, datasize is %d, using uint32", i ,proptype, size-PTP_ece_Prop_Val_Data);
					if ((size-PTP_ece_Prop_Val_Data) % sizeof(uint32_t) != 0)
						ptp_debug (params, "event %d: Warning: datasize modulo sizeof(uint32) is not 0: ", i, (size-PTP_ece_Prop_Val_Data) % sizeof(uint32_t) );
					for (j=0;j<(size-PTP_ece_Prop_Val_Data)/sizeof(uint32_t);j++)
						ptp_debug (params, "    %d: 0x%8x", j, dtoh32a(xdata+j*4));
					break;
				/* ImageFormat properties have to be ignored here, see special handling below */
				case PTP_DPC_CANON_EOS_ImageFormat:
				case PTP_DPC_CANON_EOS_ImageFormatCF:
				case PTP_DPC_CANON_EOS_ImageFormatSD:
				case PTP_DPC_CANON_EOS_ImageFormatExtHD:
				case PTP_DPC_CANON_EOS_CustomFuncEx:
				case PTP_DPC_CANON_EOS_FocusInfoEx:
					break;
				default:
					ptp_debug (params, "event %d: Unknown EOS property %04x, datasize is %d", i ,proptype, size-PTP_ece_Prop_Val_Data);
					for (j=0;j<size-PTP_ece_Prop_Val_Data;j++)
						ptp_debug (params, "    %d: %02x", j, xdata[j]);
					break;
				}
				switch (dpd->DataType) {
				case PTP_DTC_UINT32:
					dpd->FactoryDefaultValue.u32	= dtoh32a(xdata);
					dpd->CurrentValue.u32		= dtoh32a(xdata);
					ptp_debug (params ,"event %d: currentvalue of %x is %x", i, proptype, dpd->CurrentValue.u32);
					break;
				case PTP_DTC_INT16:
					dpd->FactoryDefaultValue.i16	= dtoh16a(xdata);
					dpd->CurrentValue.i16		= dtoh16a(xdata);
					ptp_debug (params,"event %d: currentvalue of %x is %d", i, proptype, dpd->CurrentValue.i16);
					break;
				case PTP_DTC_UINT16:
					dpd->FactoryDefaultValue.u16	= dtoh16a(xdata);
					dpd->CurrentValue.u16		= dtoh16a(xdata);
					ptp_debug (params,"event %d: currentvalue of %x is %x", i, proptype, dpd->CurrentValue.u16);
					break;
				case PTP_DTC_UINT8:
					dpd->FactoryDefaultValue.u8	= dtoh8a(xdata);
					dpd->CurrentValue.u8		= dtoh8a(xdata);
					ptp_debug (params,"event %d: currentvalue of %x is %x", i, proptype, dpd->CurrentValue.u8);
					break;
				case PTP_DTC_INT8:
					dpd->FactoryDefaultValue.i8	= dtoh8a(xdata);
					dpd->CurrentValue.i8		= dtoh8a(xdata);
					ptp_debug (params,"event %d: currentvalue of %x is %x", i, proptype, dpd->CurrentValue.i8);
					break;
				case PTP_DTC_STR: {
#if 0 /* 5D MII and 400D aktually store plain ASCII in their string properties */
					uint8_t len = 0;
					dpd->FactoryDefaultValue.str	= ptp_unpack_string(params, data, 0, &len);
					dpd->CurrentValue.str		= ptp_unpack_string(params, data, 0, &len);
#else
					free (dpd->FactoryDefaultValue.str);
					dpd->FactoryDefaultValue.str	= strdup( (char*)xdata );

					free (dpd->CurrentValue.str);
					dpd->CurrentValue.str		= strdup( (char*)xdata );
#endif
					ptp_debug (params,"event %d: currentvalue of %x is %s", i, proptype, dpd->CurrentValue.str);
					break;
				}
				default:
					/* debug is printed in switch above this one */
					break;
				}

				/* ImageFormat and customFuncEx special handling (WARNING: dont move this in front of the dpd->DataType switch!) */
				switch (proptype) {
				case PTP_DPC_CANON_EOS_ImageFormat:
				case PTP_DPC_CANON_EOS_ImageFormatCF:
				case PTP_DPC_CANON_EOS_ImageFormatSD:
				case PTP_DPC_CANON_EOS_ImageFormatExtHD:
					dpd->DataType = PTP_DTC_UINT16;
					dpd->FactoryDefaultValue.u16	= ptp_unpack_EOS_ImageFormat( params, &xdata );
					dpd->CurrentValue.u16		= dpd->FactoryDefaultValue.u16;
					ptp_debug (params,"event %d: decoded imageformat, currentvalue of %x is %x", i, proptype, dpd->CurrentValue.u16);
					break;
				case PTP_DPC_CANON_EOS_CustomFuncEx:
					dpd->DataType = PTP_DTC_STR;
					free (dpd->FactoryDefaultValue.str);
					free (dpd->CurrentValue.str);
					dpd->FactoryDefaultValue.str	= ptp_unpack_EOS_CustomFuncEx( params, &xdata );
					dpd->CurrentValue.str		= strdup( (char*)dpd->FactoryDefaultValue.str );
					ptp_debug (params,"event %d: decoded custom function, currentvalue of %x is %s", i, proptype, dpd->CurrentValue.str);
					break;
				case PTP_DPC_CANON_EOS_FocusInfoEx:
					dpd->DataType = PTP_DTC_STR;
					free (dpd->FactoryDefaultValue.str);
					free (dpd->CurrentValue.str);
					dpd->FactoryDefaultValue.str	= ptp_unpack_EOS_FocusInfoEx( params, &xdata, size );
					dpd->CurrentValue.str		= strdup( (char*)dpd->FactoryDefaultValue.str );
					ptp_debug (params,"event %d: decoded focus info, currentvalue of %x is %s", i, proptype, dpd->CurrentValue.str);
					break;
				}
			}
			break;
		/* one more information record handed to us */
		/* Versions seen: (d199)
		 * 100D: 	7 (original reference)
		 * 5d Mark 3:	7
		 * 650D:	7
		 * 6D:		7
		 * M10:		8
		 * 70D:		8
		 * 5Dsr:	b
		 * 200D: 	f
		 * EOS R:	11
		 */
		case PTP_EC_CANON_EOS_OLCInfoChanged: {
			uint32_t		len, curoff;
			uint16_t		mask,proptype;
			PTPDevicePropDesc	*dpd;
			int			olcver = 0;

			dpd = _lookup_or_allocate_canon_prop(params, PTP_DPC_CANON_EOS_OLCInfoVersion);
			if (dpd) {
				ptp_debug (params, "olcinfoversion is %d", dpd->CurrentValue.u32);
				olcver = dpd->CurrentValue.u32;
			}

			/* unclear what OLC stands for */
			ptp_debug (params, "event %d: EOS event OLCInfoChanged (size %d)", i, size);
			if (size >= 0x8) {	/* event info */
				unsigned int k;
				for (k=8;k<size;k++)
					ptp_debug (params, "    %d: %02x", k-8, curdata[k]);
			}
			len = dtoh32a(curdata+8);
			if ((len != size-8) && (len != size-4)) {
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = strdup("OLC size unexpected");
				ptp_debug (params, "event %d: OLC unexpected size %d for blob len %d (not -4 nor -8)", i, size, len);
				break;
			}
			mask = dtoh16a(curdata+8+4);
			if (size < 14) {
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = strdup("OLC size too small");
				ptp_debug (params, "event %d: OLC unexpected size %d", i, size);
				break;
			}
			curoff = 8+4+4;
			if (mask & CANON_EOS_OLC_BUTTON) {
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = malloc(strlen("Button 1234567"));
				sprintf(ce[i].u.info, "Button %d",  dtoh16a(curdata+curoff));
				i++;
				curoff += 2; /* 7, 8 , f */
			}

			if (mask & CANON_EOS_OLC_SHUTTERSPEED) {
				/* 6 bytes: 01 01 98 10 00 60 */
				/* this seesm to be the shutter speed record */
				/* EOS 200D seems to have 7 bytes here, sample:
				 * 7 bytes: 01 03 98 10 00 70 00
				 * EOS R also 7 bytes
				 * 7 bytes: 01 01 a0 0c 00 0c 00
				 */
				proptype = PTP_DPC_CANON_EOS_ShutterSpeed;
				dpd = _lookup_or_allocate_canon_prop(params, proptype);
				dpd->CurrentValue.u16 = curdata[curoff+5]; /* just use last byte */

				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_PROPERTY;
				ce[i].u.propid = proptype;
				/* hack to differ between older EOS and EOS 200D newer */
				switch (olcver) {
				case 0xf:
				case 0x11:
					curoff += 7;	/* f (200D), 8 (M10) ???, 11 is EOS R */
					break;
				case 0x7:
				case 0x8: /* EOS 70D */
				case 0xb: /* EOS 5Ds */
					curoff += 6;	/* 7 , b (5ds) */
					break;
				default:
					if (olcver >= 0xf)
						curoff += 7;
					else
						curoff += 6;
					break;
				}
				i++;
			}
			if (mask & CANON_EOS_OLC_APERTURE) {
				/* 5 bytes: 01 01 5b 30 30 */
				/* this seesm to be the aperture record */
				/* EOS 200D seems to have 6 bytes here?
				 * 6 bytes: 01 01 50 20 20 00 *
				 */
				proptype = PTP_DPC_CANON_EOS_Aperture;
				dpd = _lookup_or_allocate_canon_prop(params, proptype);
				dpd->CurrentValue.u16 = curdata[curoff+4]; /* just use last byte */

				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_PROPERTY;
				ce[i].u.propid = proptype;
				if (olcver >= 0xf) {
					curoff += 6;	/* f, 11 */
				} else {
					curoff += 5;	/* 7, 8, b */
				}
				i++;
			}
			if (mask & CANON_EOS_OLC_ISO) {
				/* 4 bytes: 01 01 00 78 */
				/* this seesm to be the aperture record */
				proptype = PTP_DPC_CANON_EOS_ISOSpeed;
				dpd = _lookup_or_allocate_canon_prop(params, proptype);
				dpd->CurrentValue.u16 = curdata[curoff+3]; /* just use last byte */

				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_PROPERTY;
				ce[i].u.propid = proptype;
				curoff += 4;	/* 7, 8, b, f*/
				i++;
			}
			if (mask & 0x0010) {
				/* mask 0x0010: 4 bytes, 04 00 00 00 observed */
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = malloc(strlen("OLCInfo event 0x0010 content 01234567")+1);
				sprintf(ce[i].u.info,"OLCInfo event 0x0010 content %02x%02x%02x%02x",
					curdata[curoff],
					curdata[curoff+1],
					curdata[curoff+2],
					curdata[curoff+3]
				);
				curoff += 4;
				i++;
			}
			if (mask & 0x0020) {
				/* mask 0x0020: 6 bytes, 00 00 00 00 00 00 observed.
				 * This seems to be the self-timer record: when active,
				 * has the form of 00 00 01 00 XX XX, where the last two bytes
				 * stand for the number of seconds remaining until the shot */
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = malloc(strlen("OLCInfo event 0x0020 content 0123456789ab")+1);
				sprintf(ce[i].u.info,"OLCInfo event 0x0020 content %02x%02x%02x%02x%02x%02x",
					curdata[curoff],
					curdata[curoff+1],
					curdata[curoff+2],
					curdata[curoff+3],
					curdata[curoff+4],
					curdata[curoff+5]
				);
				curoff += 6;
				i++;
			}
			if (mask & 0x0040) {
				int	value = (signed char)curdata[curoff+2];
				/* mask 0x0040: 7 bytes, 01 01 00 00 00 00 00 observed */
				/* exposure indicator */
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = malloc(strlen("OLCInfo exposure indicator 012345678901234567890123456789abcd")+1);
				sprintf(ce[i].u.info,"OLCInfo exposure indicator %d,%d,%d.%d (%02x%02x%02x%02x)",
					curdata[curoff],
					curdata[curoff+1],
					value/10,abs(value)%10,
					curdata[curoff+3],
					curdata[curoff+4],
					curdata[curoff+5],
					curdata[curoff+6]
				);
				curoff += 7;
				i++;
			}
			if (mask & 0x0080) {
				/* mask 0x0080: 4 bytes, 00 00 00 00 observed */
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = malloc(strlen("OLCInfo event 0x0080 content 01234567")+1);
				sprintf(ce[i].u.info,"OLCInfo event 0x0080 content %02x%02x%02x%02x",
					curdata[curoff],
					curdata[curoff+1],
					curdata[curoff+2],
					curdata[curoff+3]
				);
				curoff += 4;
				i++;
			}
			if (mask & 0x0100) {
				/* mask 0x0100: 6 bytes, 00 00 00 00 00 00 (before focus) and 00 00 00 00 01 00 (on focus) observed */
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_FOCUSINFO;
				ce[i].u.info = malloc(strlen("0123456789ab")+1);
				sprintf(ce[i].u.info,"%02x%02x%02x%02x%02x%02x",
					curdata[curoff],
					curdata[curoff+1],
					curdata[curoff+2],
					curdata[curoff+3],
					curdata[curoff+4],
					curdata[curoff+5]
				);
				curoff += 6;
				i++;
			}
			if (mask & 0x0200) {
				/* mask 0x0200: 7 bytes, 00 00 00 00 00 00 00 observed */
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_FOCUSMASK;
				ce[i].u.info = malloc(strlen("0123456789abcd0123456789abcdef")+1);
				sprintf(ce[i].u.info,"%02x%02x%02x%02x%02x%02x%02x",
					curdata[curoff],
					curdata[curoff+1],
					curdata[curoff+2],
					curdata[curoff+3],
					curdata[curoff+4],
					curdata[curoff+5],
					curdata[curoff+6]
				);
				curoff += 7;
				i++;
			}
			if (mask & 0x0400) {
				/* mask 0x0400: 7 bytes, 00 00 00 00 00 00 00 observed */
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = malloc(strlen("OLCInfo event 0x0400 content 0123456789abcd")+1);
				sprintf(ce[i].u.info,"OLCInfo event 0x0400 content %02x%02x%02x%02x%02x%02x%02x",
					curdata[curoff],
					curdata[curoff+1],
					curdata[curoff+2],
					curdata[curoff+3],
					curdata[curoff+4],
					curdata[curoff+5],
					curdata[curoff+6]
				);
				curoff += 7;
				i++;
			}
			if (mask & 0x0800) {
				/* mask 0x0800: 8 bytes, 00 00 00 00 00 00 00 00 and 19 01 00 00 00 00 00 00 and others observed */
				/*   might be mask of focus points selected */
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = malloc(strlen("OLCInfo event 0x0800 content 0123456789abcdef")+1);
				sprintf(ce[i].u.info,"OLCInfo event 0x0800 content %02x%02x%02x%02x%02x%02x%02x%02x",
					curdata[curoff],
					curdata[curoff+1],
					curdata[curoff+2],
					curdata[curoff+3],
					curdata[curoff+4],
					curdata[curoff+5],
					curdata[curoff+6],
					curdata[curoff+7]
				);
				curoff += 8;
				i++;
			}
			if (mask & 0x1000) {
				/* mask 0x1000: 1 byte, 00 observed */
				ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
				ce[i].u.info = malloc(strlen("OLCInfo event 0x1000 content 01")+1);
				sprintf(ce[i].u.info,"OLCInfo event 0x1000 content %02x",
					curdata[curoff]
				);
				curoff += 1;
				i++;
			}
			/* handle more masks */
			ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
			ce[i].u.info = malloc(strlen("OLCInfo event mask 0123456789")+1);
			sprintf(ce[i].u.info, "OLCInfo event mask=%x",  mask);
			break;
		}
		case PTP_EC_CANON_EOS_CameraStatusChanged:
			ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_CAMERASTATUS;
			ce[i].u.status =  dtoh32a(curdata+8);
			ptp_debug (params, "event %d: EOS event CameraStatusChanged (size %d) = %d", i, size, dtoh32a(curdata+8));
			params->eos_camerastatus = dtoh32a(curdata+8);
			break;
		case 0: /* end marker */
			if (size == 8) /* no output */
				break;
			ptp_debug (params, "event %d: EOS event 0, but size %d", i, size);
			break;
		case PTP_EC_CANON_EOS_BulbExposureTime:
			ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
			ce[i].u.info = malloc(strlen("BulbExposureTime 123456789012345678"));
			sprintf (ce[i].u.info, "BulbExposureTime %u",  dtoh32a(curdata+8));
			break;
		case PTP_EC_CANON_EOS_ObjectRemoved:
			ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_OBJECTREMOVED;
			ce[i].u.object.oid = dtoh32a(curdata+8);
			break;
		default:
			switch (type) {
#define XX(x)		case PTP_EC_CANON_EOS_##x: 								\
				ptp_debug (params, "event %u: unhandled EOS event "#x" (size %u)", i, size); 	\
				ce[i].u.info = malloc(strlen("unhandled EOS event "#x" (size 12345678901)")+1);	\
				sprintf (ce[i].u.info, "unhandled EOS event "#x" (size %u)",  size);		\
				break;
			XX(RequestGetEvent)
			XX(RequestGetObjectInfoEx)
			XX(StorageStatusChanged)
			XX(StorageInfoChanged)
			XX(ObjectInfoChangedEx)
			XX(ObjectContentChanged)
			XX(WillSoonShutdown)
			XX(ShutdownTimerUpdated)
			XX(RequestCancelTransfer)
			XX(RequestObjectTransferDT)
			XX(RequestCancelTransferDT)
			XX(StoreAdded)
			XX(StoreRemoved)
			XX(BulbExposureTime)
			XX(RecordingTime)
			XX(RequestObjectTransferTS)
			XX(AfResult)
			XX(PowerZoomInfoChanged)
			XX(CTGInfoCheckComplete)
#undef XX
			default:
				ptp_debug (params, "event %d: unknown EOS event %04x", i, type);
				break;
			}
			if (size >= 0x8) {	/* event info */
				unsigned int j;
				/*ptp_debug (params, "data=%p, curdata=%p, datsize=%d, size=%d", data, curdata, datasize, size);*/
				for (j=8;j<size;j++)
					ptp_debug (params, "    %d: %02x", j, curdata[j]);
			}
			ce[i].type = PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN;
			break;
		}
		curdata += size;
		i++;
		if (i >= entries) {
			ptp_debug (params, "BAD: i %d, entries %d", i, entries);
		}
	}
	if (!i) {
		free (ce);
		ce = NULL;
	}
	*pce = ce;
	return i;
}

/*
    PTP USB Event container unpack for Nikon events.
*/
#define PTP_nikon_ec_Length		0
#define PTP_nikon_ec_Code		2
#define PTP_nikon_ec_Param1		4
#define PTP_nikon_ec_Size		6
static inline void
ptp_unpack_Nikon_EC (PTPParams *params, unsigned char* data, unsigned int len, PTPContainer **ec, unsigned int *cnt)
{
	unsigned int i;

	*ec = NULL;
	if (data == NULL)
		return;
	if (len < PTP_nikon_ec_Code)
		return;
	*cnt = dtoh16a(&data[PTP_nikon_ec_Length]);
	if (*cnt > (len-PTP_nikon_ec_Code)/PTP_nikon_ec_Size) { /* broken cnt? */
		*cnt = 0;
		return;
	}
	if (!*cnt)
		return;

	*ec = malloc(sizeof(PTPContainer)*(*cnt));

	for (i=0;i<*cnt;i++) {
		memset(&(*ec)[i],0,sizeof(PTPContainer));
		(*ec)[i].Code	= dtoh16a(&data[PTP_nikon_ec_Code+PTP_nikon_ec_Size*i]);
		(*ec)[i].Param1	= dtoh32a(&data[PTP_nikon_ec_Param1+PTP_nikon_ec_Size*i]);
		(*ec)[i].Nparam	= 1;
	}
}

/*
 *  PTP USB Event container unpack for Nikon events, 2nd generation.
 */
#define PTP_nikon_ec_ex_Length		0
#define PTP_nikon_ec_ex_Code		2

static inline int
ptp_unpack_Nikon_EC_EX (PTPParams *params, unsigned char* data, unsigned int len, PTPContainer **ec, unsigned int *cnt)
{
	unsigned int i, offset;

	*ec = NULL;
	if (data == NULL)
		return 0;
	if (len < PTP_nikon_ec_ex_Code)
		return 0;
	*cnt = dtoh16a(&data[PTP_nikon_ec_ex_Length]);
	if (*cnt > (len-PTP_nikon_ec_ex_Code)/4) { /* broken cnt? simple first check ... due to dynamic size, we need to do more later */
		*cnt = 0;
		return 0;
	}
	if (!*cnt)
		return 1;

	*ec = malloc(sizeof(PTPContainer)*(*cnt));
	offset = PTP_nikon_ec_ex_Code+sizeof(uint16_t);

	for (i=0;i<*cnt;i++) {
		memset(&(*ec)[i],0,sizeof(PTPContainer));
		if (len - offset < 4) {
			free (*ec);
			*ec = NULL;
			*cnt = 0;
			return 0;
		}
		(*ec)[i].Code	= dtoh16a(&data[offset]);
		(*ec)[i].Nparam	= dtoh16a(&data[offset+2]);
		ptp_debug (params, "nikon eventex %d: code 0x%04x, params %d", i, (*ec)[i].Code, (*ec)[i].Nparam);
		if (	((*ec)[i].Nparam > 5) 					||
			(len < ((*ec)[i].Nparam*sizeof(uint32_t)) + 4 + offset)
		) {
			free (*ec);
			*ec = NULL;
			*cnt = 0;
			return 0;
		}
		switch ((*ec)[i].Nparam) {
		case 5:	(*ec)[i].Param5	= dtoh32a(&data[offset+4+sizeof(uint32_t)*4]);/* fallthrough */
		case 4:	(*ec)[i].Param4	= dtoh32a(&data[offset+4+sizeof(uint32_t)*3]);/* fallthrough */
		case 3:	(*ec)[i].Param3	= dtoh32a(&data[offset+4+sizeof(uint32_t)*2]);/* fallthrough */
		case 2:	(*ec)[i].Param2	= dtoh32a(&data[offset+4+sizeof(uint32_t)*1]);/* fallthrough */
		case 1:	(*ec)[i].Param1	= dtoh32a(&data[offset+4]);
			/* fallthrough */
		case 0:	break;
		}
		offset += (*ec)[i].Nparam*sizeof(uint32_t) + 4;
	}
	return 1;
}

static inline uint32_t
ptp_pack_EK_text(PTPParams *params, PTPEKTextParams *text, unsigned char **data) {
	int i, len = 0;
	uint8_t	retlen;
	unsigned char *curdata;

	len =	2*(strlen(text->title)+1)+1+
		2*(strlen(text->line[0])+1)+1+
		2*(strlen(text->line[1])+1)+1+
		2*(strlen(text->line[2])+1)+1+
		2*(strlen(text->line[3])+1)+1+
		2*(strlen(text->line[4])+1)+1+
		4*2+2*4+2+4+2+5*4*2;
	*data = malloc(len);
	if (!*data) return 0;

	curdata = *data;
	htod16a(curdata,100);curdata+=2;
	htod16a(curdata,1);curdata+=2;
	htod16a(curdata,0);curdata+=2;
	htod16a(curdata,1000);curdata+=2;

	htod32a(curdata,0);curdata+=4;
	htod32a(curdata,0);curdata+=4;

	htod16a(curdata,6);curdata+=2;
	htod32a(curdata,0);curdata+=4;

	ptp_pack_string(params, text->title, curdata, 0, &retlen); curdata+=2*retlen+1;htod16a(curdata,0);curdata+=2;
	htod16a(curdata,0x10);curdata+=2;

	for (i=0;i<5;i++) {
		ptp_pack_string(params, text->line[i], curdata, 0, &retlen); curdata+=2*retlen+1;htod16a(curdata,0);curdata+=2;
		htod16a(curdata,0x10);curdata+=2;
		htod16a(curdata,0x01);curdata+=2;
		htod16a(curdata,0x02);curdata+=2;
		htod16a(curdata,0x06);curdata+=2;
	}
	return len;
}

#define ptp_canon_dir_version	0x00
#define ptp_canon_dir_ofc	0x02
#define ptp_canon_dir_unk1	0x04
#define ptp_canon_dir_objectid	0x08
#define ptp_canon_dir_parentid	0x0c
#define ptp_canon_dir_previd	0x10	/* in same dir */
#define ptp_canon_dir_nextid	0x14	/* in same dir */
#define ptp_canon_dir_nextchild	0x18	/* down one dir */
#define ptp_canon_dir_storageid	0x1c	/* only in storage entry */
#define ptp_canon_dir_name	0x20
#define ptp_canon_dir_flags	0x2c
#define ptp_canon_dir_size	0x30
#define ptp_canon_dir_unixtime	0x34
#define ptp_canon_dir_year	0x38
#define ptp_canon_dir_month	0x39
#define ptp_canon_dir_mday	0x3a
#define ptp_canon_dir_hour	0x3b
#define ptp_canon_dir_minute	0x3c
#define ptp_canon_dir_second	0x3d
#define ptp_canon_dir_unk2	0x3e
#define ptp_canon_dir_thumbsize	0x40
#define ptp_canon_dir_width	0x44
#define ptp_canon_dir_height	0x48

static inline uint16_t
ptp_unpack_canon_directory (
	PTPParams		*params,
	unsigned char		*dir,
	uint32_t		cnt,
	PTPObjectHandles	*handles,
	PTPObjectInfo		**oinfos,	/* size(handles->n) */
	uint32_t		**flags		/* size(handles->n) */
) {
	unsigned int	i, j, nrofobs = 0, curob = 0;

#define ISOBJECT(ptr) (dtoh32a((ptr)+ptp_canon_dir_storageid) == 0xffffffff)
	for (i=0;i<cnt;i++)
		if (ISOBJECT(dir+i*0x4c)) nrofobs++;
	handles->n = nrofobs;
	handles->Handler = calloc(nrofobs,sizeof(handles->Handler[0]));
	if (!handles->Handler) return PTP_RC_GeneralError;
	*oinfos = calloc(nrofobs,sizeof((*oinfos)[0]));
	if (!*oinfos) return PTP_RC_GeneralError;
	*flags  = calloc(nrofobs,sizeof((*flags)[0]));
	if (!*flags) return PTP_RC_GeneralError;

	/* Migrate data into objects ids, handles into
	 * the object handler array.
	 */
	curob = 0;
	for (i=0;i<cnt;i++) {
		unsigned char	*cur = dir+i*0x4c;
		PTPObjectInfo	*oi = (*oinfos)+curob;

		if (!ISOBJECT(cur))
			continue;

		handles->Handler[curob] = dtoh32a(cur + ptp_canon_dir_objectid);
		oi->StorageID		= 0xffffffff;
		oi->ObjectFormat	= dtoh16a(cur + ptp_canon_dir_ofc);
		oi->ParentObject	= dtoh32a(cur + ptp_canon_dir_parentid);
		oi->Filename		= strdup((char*)(cur + ptp_canon_dir_name));
		oi->ObjectCompressedSize= dtoh32a(cur + ptp_canon_dir_size);
		oi->ThumbCompressedSize	= dtoh32a(cur + ptp_canon_dir_thumbsize);
		oi->ImagePixWidth	= dtoh32a(cur + ptp_canon_dir_width);
		oi->ImagePixHeight	= dtoh32a(cur + ptp_canon_dir_height);
		oi->CaptureDate		= oi->ModificationDate = dtoh32a(cur + ptp_canon_dir_unixtime);
		(*flags)[curob]		= dtoh32a(cur + ptp_canon_dir_flags);
		curob++;
	}
	/* Walk over Storage ID entries and distribute the IDs to
	 * the parent objects. */
	for (i=0;i<cnt;i++) {
		unsigned char	*cur = dir+i*0x4c;
		uint32_t	nextchild = dtoh32a(cur + ptp_canon_dir_nextchild);

		if (ISOBJECT(cur))
			continue;
		for (j=0;j<handles->n;j++) if (nextchild == handles->Handler[j]) break;
		if (j == handles->n) continue;
		(*oinfos)[j].StorageID = dtoh32a(cur + ptp_canon_dir_storageid);
	}
	/* Walk over all objects and distribute the storage ids */
	while (1) {
		unsigned int changed = 0;
		for (i=0;i<cnt;i++) {
			unsigned char	*cur = dir+i*0x4c;
			uint32_t	oid = dtoh32a(cur + ptp_canon_dir_objectid);
			uint32_t	nextoid = dtoh32a(cur + ptp_canon_dir_nextid);
			uint32_t	nextchild = dtoh32a(cur + ptp_canon_dir_nextchild);
			uint32_t	storageid;

			if (!ISOBJECT(cur))
				continue;
			for (j=0;j<handles->n;j++) if (oid == handles->Handler[j]) break;
			if (j == handles->n) {
				/*fprintf(stderr,"did not find oid in lookup pass for current oid\n");*/
				continue;
			}
	 		storageid = (*oinfos)[j].StorageID;
			if (storageid == 0xffffffff) continue;
			if (nextoid != 0xffffffff) {
				for (j=0;j<handles->n;j++) if (nextoid == handles->Handler[j]) break;
				if (j == handles->n) {
					/*fprintf(stderr,"did not find oid in lookup pass for next oid\n");*/
					continue;
				}
				if ((*oinfos)[j].StorageID == 0xffffffff) {
					(*oinfos)[j].StorageID = storageid;
					changed++;
				}
			}
			if (nextchild != 0xffffffff) {
				for (j=0;j<handles->n;j++) if (nextchild == handles->Handler[j]) break;
				if (j == handles->n) {
					/*fprintf(stderr,"did not find oid in lookup pass for next child\n");*/
					continue;
				}
				if ((*oinfos)[j].StorageID == 0xffffffff) {
					(*oinfos)[j].StorageID = storageid;
					changed++;
				}
			}
		}
		/* Check if we:
		 * - changed no entry (nothing more to do)
		 * - changed all of them at once (usually happens)
		 * break if we do.
		 */
		if (!changed || (changed==nrofobs-1))
			break;
	}
#undef ISOBJECT
	return PTP_RC_OK;
}

static inline int
ptp_unpack_ptp11_manifest (
	PTPParams		*params,
	unsigned char		*data,
	unsigned int 		datalen,
	uint64_t		*numoifs,
	PTPObjectFilesystemInfo	**oifs
) {
	uint64_t		numberoifs, i;
	unsigned int		curoffset;
	PTPObjectFilesystemInfo	*xoifs;

	if (datalen < 8)
		return 0;
	numberoifs = dtoh64ap(params,data);
	curoffset = 8;
	xoifs = calloc(numberoifs,sizeof(PTPObjectFilesystemInfo));
	if (!xoifs)
		return 0;

	for (i = 0; i < numberoifs; i++) {
		uint8_t len,dlen;
		char *modify_date;
		PTPObjectFilesystemInfo *oif = xoifs+i;

		if (curoffset + 34 + 2 > datalen)
			goto tooshort;

		oif->ObjectHandle		= dtoh32ap(params,data+curoffset);
		oif->StorageID 			= dtoh32ap(params,data+curoffset+4);
		oif->ObjectFormat 		= dtoh16ap(params,data+curoffset+8);
		oif->ProtectionStatus 		= dtoh16ap(params,data+curoffset+10);
		oif->ObjectCompressedSize64 	= dtoh64ap(params,data+curoffset+12);
		oif->ParentObject 		= dtoh32ap(params,data+curoffset+20);
		oif->AssociationType 		= dtoh16ap(params,data+curoffset+24);
		oif->AssociationDesc 		= dtoh32ap(params,data+curoffset+26);
		oif->SequenceNumber 		= dtoh32ap(params,data+curoffset+30);
		if (!ptp_unpack_string(params, data, curoffset+34, datalen, &len, &oif->Filename))
			goto tooshort;
		if (curoffset+34+len*2+1 > datalen)
			goto tooshort;

		if (!ptp_unpack_string(params, data, curoffset+len*2+1+34, datalen, &dlen, &modify_date))
			goto tooshort;

		oif->ModificationDate 		= ptp_unpack_PTPTIME(modify_date);
		free(modify_date);
		curoffset += 34+len*2+dlen*2+2;
	}
	*numoifs = numberoifs;
	*oifs = xoifs;
	return 1;
tooshort:
	for (i = 0; i < numberoifs; i++)
		if (xoifs[i].Filename) free (xoifs[i].Filename);
	free (xoifs);
	return 0;
}

static inline void
ptp_unpack_chdk_lv_data_header (PTPParams *params, unsigned char* data, lv_data_header *header)
{
	int off = 0;
	if (data==NULL)
		return;
	header->version_major = dtoh32a(&data[off]);
	header->version_minor = dtoh32a(&data[off+=4]);
	header->lcd_aspect_ratio = dtoh32a(&data[off+=4]);
	header->palette_type = dtoh32a(&data[off+=4]);
	header->palette_data_start = dtoh32a(&data[off+=4]);
	header->vp_desc_start = dtoh32a(&data[off+=4]);
	header->bm_desc_start = dtoh32a(&data[off+=4]);
	if (header->version_minor > 1)
		header->bmo_desc_start = dtoh32a(&data[off+=4]);
}

static inline void
ptp_unpack_chdk_lv_framebuffer_desc (PTPParams *params, unsigned char* data, lv_framebuffer_desc *fd)
{
	int off = 0;
	if (data==NULL)
		return;
	fd->fb_type = dtoh32a(&data[off]);
	fd->data_start = dtoh32a(&data[off+=4]);
	fd->buffer_width = dtoh32a(&data[off+=4]);
	fd->visible_width = dtoh32a(&data[off+=4]);
	fd->visible_height = dtoh32a(&data[off+=4]);
	fd->margin_left = dtoh32a(&data[off+=4]);
	fd->margin_top = dtoh32a(&data[off+=4]);
	fd->margin_right = dtoh32a(&data[off+=4]);
	fd->margin_bot = dtoh32a(&data[off+=4]);
}

static inline int
ptp_unpack_StreamInfo (PTPParams *params, unsigned char *data, PTPStreamInfo *si, unsigned int size) {
	if (!data) return PTP_RC_GeneralError;
	if (size < 36) return PTP_RC_GeneralError;

	si->DatasetSize		= dtoh64ap(params,data+0);
	si->TimeResolution	= dtoh64ap(params,data+8);
	si->FrameHeaderSize	= dtoh32ap(params,data+16);
	si->FrameMaxSize	= dtoh32ap(params,data+20);
	si->PacketHeaderSize	= dtoh32ap(params,data+24);
	si->PacketMaxSize	= dtoh32ap(params,data+28);
	si->PacketAlignment	= dtoh32ap(params,data+32);
	return PTP_RC_OK;
}
