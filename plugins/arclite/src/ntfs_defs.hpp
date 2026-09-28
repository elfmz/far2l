#pragma once

#define SECURITY_DESCRIPTOR_REVISION 1

#define SE_OWNER_DEFAULTED	0x0001
#define SE_GROUP_DEFAULTED	0x0002
#define SE_DACL_PRESENT		0x0004
#define SE_DACL_DEFAULTED	0x0008
#define SE_SACL_PRESENT		0x0010
#define SE_SACL_DEFAULTED	0x0020
#define SE_SELF_RELATIVE	0x8000

#define ACCESS_ALLOWED_ACE_TYPE		0x00
#define ACCESS_DENIED_ACE_TYPE		0x01
#define SYSTEM_AUDIT_ACE_TYPE		0x02

#define OBJECT_INHERIT_ACE			0x01
#define CONTAINER_INHERIT_ACE		0x02
#define NO_PROPAGATE_INHERIT_ACE	0x04
#define INHERIT_ONLY_ACE			0x08
#define INHERITED_ACE				0x10

#pragma pack(push, 1)
typedef struct _SID_IDENTIFIER_AUTHORITY {
	uint8_t Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;

typedef struct _SID {
	uint8_t Revision;
	uint8_t SubAuthorityCount;
	SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
	uint32_t SubAuthority[1];
} SID, *PSID;

typedef struct _ACL {
	uint8_t AclRevision;
	uint8_t Sbz1;
	uint16_t AclSize;
	uint16_t AceCount;
	uint16_t Sbz2;
} ACL, *PACL;

typedef struct _ACE_HEADER {
	uint8_t AceType;
	uint8_t AceFlags;
	uint16_t AceSize;
} ACE_HEADER, *PACE_HEADER;

typedef struct _ACCESS_ALLOWED_ACE {
	ACE_HEADER Header;
	uint32_t Mask;
	uint32_t SidStart;
} ACCESS_ALLOWED_ACE, *PACCESS_ALLOWED_ACE;

typedef struct _SECURITY_DESCRIPTOR_RELATIVE {
	uint8_t Revision;
	uint8_t Sbz1;
	uint16_t Control;
	uint32_t Owner;
	uint32_t Group;
	uint32_t Sacl;
	uint32_t Dacl;
} SECURITY_DESCRIPTOR_RELATIVE, *PSECURITY_DESCRIPTOR_RELATIVE;
#pragma pack(pop)

typedef struct _REPARSE_DATA_BUFFER {
	ULONG  ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		} SymbolicLinkReparseBuffer;
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;
		struct {
			UCHAR  DataBuffer[1];
		} GenericReparseBuffer;
	};
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
