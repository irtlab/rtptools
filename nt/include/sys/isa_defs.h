/*
 * Copyright (c) 1993-1994, by Sun Microsystems, Inc.
 */

#ifndef	_SYS_ISA_DEFS_H
#define	_SYS_ISA_DEFS_H

/*
 * This header file serves to group a set of well known defines and to
 * set these for each instruction set architecture.  These defines may
 * be divided into two groups;  characteristics of the processor and
 * implementation choices for Solaris on a processor.
 *
 * Processor Characteristics:
 *
 * _LITTLE_ENDIAN / _BIG_ENDIAN:
 *	The natural byte order of the processor.  A pointer to an int points
 *	to the least/most significant byte of that int.
 *
 * _STACK_GROWS_UPWARD / _STACK_GROWS_DOWNWARD:
 *	The processor specific direction of stack growth.  A push onto the
 *	stack increases/decreases the stack pointer, so it stores data at
 *	successively higher/lower addresses.  (Stackless machines ignored
 *	without regrets).
 *
 * _LONG_LONG_HTOL / _LONG_LONG_LTOH:
 *	A pointer to a long long points to the most/least significant long
 *	within that long long.
 *
 * _BIT_FIELDS_HTOL / _BIT_FIELDS_LTOH:
 *	The C compiler assigns bit fields from the high/low to the low/high end
 *	of an int (most to least significant vs. least to most significant).
 *
 * _IEEE_754:
 *	The processor (or supported implementations of the processor)
 *	supports the ieee-754 floating point standard.  No other floating
 *	point standards are supported (or significant).  Any other supported
 *	floating point formats are expected to be cased on the ISA processor
 *	symbol.
 *
 * _CHAR_IS_UNSIGNED / _CHAR_IS_SIGNED:
 *	The C Compiler implements objects of type `char' as `unsigned' or
 *	`signed' respectively.  This is really an implementation choice of
 *	the compiler writer, but it is specified in the ABI and tends to
 *	be uniform across compilers for an instruction set architecture.
 *	Hence, it has the properties of a processor characteristic.
 *
 * _CHAR_ALIGNMENT / _SHORT_ALIGNMENT / _INT_ALIGNMENT / _LONG_ALIGNMENT /
 * _LONG_LONG_ALIGNMENT / _DOUBLE_ALIGNMENT / _LONG_DOUBLE_ALIGNMENT /
 * _POINTER_ALIGNMENT:
 *	The ABI defines alignment requirements of each of the primitive
 *	object types.  Some, if not all, may be hardware requirements as
 * 	well.  The values are expressed in "byte-alignment" units.
 *
 * _MAX_ALIGNMENT:
 *	The most stringent alignment requirement as specified by the ABI.
 *	Equal to the maximum of all the above _XXX_ALIGNMENT values.
 *
 * _ALIGNMENT_REQUIRED:
 *	True or false (1 or 0) whether or not the hardware requires the ABI
 *	alignment.
 *
 *
 * Implementation Choices:
 *
 * _SUNOS_VTOC_8 / _SUNOS_VTOC_16 / _SVR4_VTOC_16:
 *	This specifies the form of the disk VTOC (or label):
 *
 *	_SUNOS_VTOC_8:
 *		This is a VTOC form which is upwardly compatible with the
 *		SunOS 4.x disk label and allows 8 partitions per disk.
 *
 *	_SUNOS_VTOC_16:
 *		In this format the incore vtoc image matches the ondisk
 *		version.  It allows 16 slices per disk, and is not
 *		compatible with the SunOS 4.x disk label.
 *
 *	Note that these are not the only two VTOC forms possible and
 *	additional forms may be added.  One possible form would be the
 *	SVr4 VTOC form.  The symbol for that is reserved now, although
 *	it is not implemented.
 *
 *	_SVR4_VTOC_16:
 *		This VTOC form is compatible with the System V Release 4
 *		VTOC (as implemented on the SVr4 Intel and 3b ports) with
 *		16 partitions per disk.
 *
 *
 * _DMA_USES_PHYSADDR / _DMA_USES_VIRTADDR
 *	This describes the type of addresses used by system DMA:
 *
 *	_DMA_USES_PHYSADDR:
 *		This type of DMA, used in the x86 and PowerPC implementations,
 *		requires physical addresses for DMA buffers.  The 24-bit
 *		addresses used by some legacy boards is the source of the
 *		"low-memory" (<16MB) requirement for some devices using DMA.
 *
 *	_DMA_USES_VIRTADDR:
 *		This method of DMA allows the use of virtual addresses for
 *		DMA transfers.
 *
 * _FIRMWARE_NEEDS_FDISK / _NO_FDISK_PRESENT
 *      This indicates the presence/absence of an fdisk table.
 *
 *      _FIRMWARE_NEEDS_FDISK
 *              The fdisk table is required by system firmware.  If present,
 *              it allows a disk to be subdivided into multiple fdisk
 *              partitions, each of which is equivalent to a separate,
 *              virtual disk.  This enables the co-existence of multiple
 *              operating systems on a shared hard disk.
 *
 *      _NO_FDISK_PRESENT
 *              If the fdisk table is absent, it is assumed that the entire
 *              media is allocated for a single operating system.
 */

#ifdef	__cplusplus
extern "C" {
#endif


/*
 * The following set of definitions characterize the Solaris on Intel systems.
 *
 * The feature test macro __i386 is generic for all processors implementing
 * the Intel 386 instruction set or a superset of it.  Specifically, this
 * includes all members of the 386, 486, and Pentium family of processors.
 */
#if defined(__i386) || defined(i386)

/*
 * Make sure that the ANSI-C "politically correct" symbol is defined.
 */
#if !defined(__i386)
#define	__i386
#endif

/*
 * Define the appropriate "processor characteristics"
 */
#define	_LITTLE_ENDIAN
#define	_STACK_GROWS_DOWNWARD
#define	_LONG_LONG_LTOH
#define	_BIT_FIELDS_LTOH
#define	_IEEE_754
#define	_CHAR_IS_SIGNED
#define	_CHAR_ALIGNMENT		1
#define	_SHORT_ALIGNMENT	2
#define	_INT_ALIGNMENT		4
#define	_LONG_ALIGNMENT		4
#define	_LONG_LONG_ALIGNMENT	4
#define	_DOUBLE_ALIGNMENT	4
#define	_LONG_DOUBLE_ALIGNMENT	4
#define	_POINTER_ALIGNMENT	4
#define	_MAX_ALIGNMENT		4
#define	_ALIGNMENT_REQUIRED	0

/*
 * Define the appropriate "implementation choices".
 */
#define	_SUNOS_VTOC_16
#define	_DMA_USES_PHYSADDR
#define	_FIRMWARE_NEEDS_FDISK


/*
 * The following set of definitions characterize the Solaris on PowerPC systems.
 *
 * The feature test macro __ppc is generic for all processors implementing
 * the PowerPC instruction set or a superset of it.  Specifically, this
 * includes all members of the 601, 603, 604, and 620 family of processors.
 */
#elif defined(__ppc)

/*
 * Define the appropriate "processor characteristics"
 */
#define	_LITTLE_ENDIAN
#define	_STACK_GROWS_DOWNWARD
#define	_LONG_LONG_LTOH
#define	_BIT_FIELDS_LTOH
#define	_IEEE_754
#define	_CHAR_IS_UNSIGNED
#define	_CHAR_ALIGNMENT		1
#define	_SHORT_ALIGNMENT	2
#define	_INT_ALIGNMENT		4
#define	_LONG_ALIGNMENT		4
#define	_LONG_LONG_ALIGNMENT	8
#define	_DOUBLE_ALIGNMENT	8
#define	_LONG_DOUBLE_ALIGNMENT	16
#define	_POINTER_ALIGNMENT	4
#define	_MAX_ALIGNMENT		16
#define	_ALIGNMENT_REQUIRED	1

/*
 * Define the appropriate "implementation choices".
 */
#define	_SUNOS_VTOC_16
#define	_DMA_USES_PHYSADDR
#define	_FIRMWARE_NEEDS_FDISK


/*
 * The following set of definitions characterize the Solaris on SPARC system.
 *
 * The flag __sparc is only guaranteed to indicate SPARC processors version 8
 * or earlier.
 */
#elif defined(__sparc) || defined(sparc)

/*
 * Make sure that the ANSI-C "politically correct" symbol is defined.
 */
#if !defined(__sparc)
#define	__sparc
#endif

/*
 * Define the appropriate "processor characteristics"
 */
#define	_BIG_ENDIAN
#define	_STACK_GROWS_DOWNWARD
#define	_LONG_LONG_HTOL
#define	_BIT_FIELDS_HTOL
#define	_IEEE_754
#define	_CHAR_IS_SIGNED
#define	_CHAR_ALIGNMENT		1
#define	_SHORT_ALIGNMENT	2
#define	_INT_ALIGNMENT		4
#define	_LONG_ALIGNMENT		4
#define	_LONG_LONG_ALIGNMENT	8
#define	_DOUBLE_ALIGNMENT	8
#define	_LONG_DOUBLE_ALIGNMENT	8
#define	_POINTER_ALIGNMENT	4
#define	_MAX_ALIGNMENT		8
#define	_ALIGNMENT_REQUIRED	1

/*
 * Define the appropriate "implementation choices".
 */
#define	_SUNOS_VTOC_8
#define	_DMA_USES_VIRTADDR
#define	_NO_FDISK_PRESENT


/*
 * #error is strictly ansi-C, but works as well as anything for K&R systems.
 */
#else
/* 
 *#error ISA not supported
 */
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_ISA_DEFS_H */
