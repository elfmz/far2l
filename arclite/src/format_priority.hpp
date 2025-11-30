#pragma once

	// special case: UDF must go before ISO
///	prioritize(arc_entries, c_udf, c_iso);
	// special case: UDF must go before ISO
///	prioritize(arc_entries, c_udf, c_iso);
	// special case: Rar must go before Split
///	prioritize(arc_entries, c_rar, c_split);
	// special case: Dmg must go before HFS
///	prioritize(arc_entries, c_dmg, c_hfs);

const std::map<ArcType, int> g_format_priority = {

    {c_udf,     19},   // UDF — современный стандарт для DVD/Blu-ray, выше ISO
    {c_iso,     20},   // Классический ISO9660

    {c_sqfs,    21},   // SquashFS — встраиваемая FS, НЕ должна уступать xz/zstd/gzip итд
    {c_CramFS,  21},   // CramFS — старый аналог, но тоже FS
    {c_dmg,     21},   // DMG — контейнер Apple, должен быть ВЫШЕ hfs

    {c_split,   60},  // split — сырые части

    {c_vdi,     100},  // VirtualBox
    {c_vhd,     100},  // Hyper-V
    {c_vmdk,    100},  // VMware
    {c_qcow,    100},  // QEMU

    {c_mbr,     100},  // MBR
    {c_gpt,     100},  // GPT
    {c_apm,     100},  // APM

    {c_hfs,     100},  // HFS — только если не DMG
    {c_apfs,    100},  // APFS — современная Apple FS
    {c_ntfs,    100},  // NTFS
    {c_ext4,    100},  // Ext2/3/4
    {c_fat,     100},  // FAT16/FAT32
    {c_xfat,    100},  // exFAT

    {c_elf,     100},  // ELF — исполняемый, но может содержать данные
    {c_pe,      100},  // PE (Windows EXE/DLL)
    {c_macho,   100},  // Mach-O (macOS)

    {c_7z,      100},  // 7-Zip
    {c_zip,     100},  // ZIP
    {c_rar,     100},  // RAR
    {c_rar5,    100},  // RAR5
    {c_tar,     100},  // tar
    {c_wim,     100},  // WIM
    {c_xar,     100},  // XAR
    {c_rpm,     100},  // RPM
    {c_cpio,    100},  // cpio
    {c_ar,      100},  // ar
    {c_cab,     100},  // CAB
    {c_lzh,     100},  // LZH
    {c_arj,     100},  // ARJ

    {c_zstd,    100},  // zstd
    {c_xz,      100},  // xz
    {c_gzip,    100},  // gzip
    {c_bzip2,   100},  // bzip2
    {c_lzma,    100},  // lzma
    {c_z,       100},  // compress (.Z)
    {c_Ppmd,    100},  // PPMd

    {c_chm,     100},  // CHM
    {c_compound,100},  // Compound File (OLE)
    {c_swf,     100},  // SWF
    {c_flv,     100},  // FLV
    {c_ihex,    100},  // Intel HEX
    {c_mslz,    100},  // MS LZ (LZX)
    {c_Mub,     100},  // UBI/MUB
    {c_uefif,   100},  // UEFI Firmware File
    {c_uefic,   100},  // UEFI Capsule
};
