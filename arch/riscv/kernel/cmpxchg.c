/*
 * Copyright (C) 2017 Imagination Technologies
 * Author: Paul Burton <paul.burton@mips.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/bitops.h>
#include <asm/cmpxchg.h>

#define TEMPLATE_XCGH_SMALL(__func,__op)				\
unsigned long __func(volatile void *ptr, unsigned long new,		\
		     unsigned int size)					\
{									\
	u32 old32, new32, load32, mask;					\
	volatile u32 *ptr32;						\
	unsigned int shift;						\
									\
	/* Check that ptr is naturally aligned */			\
	WARN_ON((unsigned long)ptr & (size - 1));			\
									\
	/* Mask value to the correct size. */				\
	mask = GENMASK((size * BITS_PER_BYTE) - 1, 0);			\
	new &= mask;							\
									\
	/*								\
	 * Calculate a shift & mask that corresponds to the value	\
	 * we wish to exchange within the naturally aligned 4 byte 	\
	 * integer that includes it.					\
	 */								\
	shift = (unsigned long)ptr & 0x3;				\
	shift *= BITS_PER_BYTE;						\
	mask <<= shift;							\
									\
	/*								\
	 * Calculate a pointer to the naturally aligned 4 byte 		\
	 * integer that includes our byte, and load its value.		\
	 */								\
	ptr32 = (volatile u32 *)((unsigned long)ptr & ~0x3);		\
	load32 = *ptr32;						\
									\
	do {								\
		old32 = load32;						\
		new32 = (load32 & ~mask) | (new << shift);		\
		load32 = __op(ptr32, old32, new32);			\
	} while (load32 != old32);					\
									\
	return (load32 & mask) >> shift;				\
}

TEMPLATE_XCGH_SMALL(__xchg_small,cmpxchg)
TEMPLATE_XCGH_SMALL(__xchg_relaxed_small,cmpxchg_relaxed)
TEMPLATE_XCGH_SMALL(__xchg_acquire_small,cmpxchg_acquire)
TEMPLATE_XCGH_SMALL(__xchg_release_small,cmpxchg_release)

#define TEMPLATE_CMPXCGH_SMALL(__func,__op)				\
unsigned long __func(volatile void *ptr, unsigned long old,		\
		     unsigned long new, unsigned int size)		\
{									\
	u32 old32, new32, load32, mask;					\
	volatile u32 *ptr32;						\
	unsigned int shift;						\
	u32 load;							\
									\
	/* Check that ptr is naturally aligned */			\
	WARN_ON((unsigned long)ptr & (size - 1));			\
									\
	/* Mask inputs to the correct size. */				\
	mask = GENMASK((size * BITS_PER_BYTE) - 1, 0);			\
	old &= mask;							\
	new &= mask;							\
									\
	/*								\
	 * Calculate a shift & mask that corresponds to the value	\
	 * we wish to exchange within the naturally aligned 4 byte 	\
	 * integer that includes it.					\
	 */								\
	shift = (unsigned long)ptr & 0x3;				\
	shift *= BITS_PER_BYTE;						\
	mask <<= shift;							\
									\
	/*								\
	 * Calculate a pointer to the naturally aligned 4 byte 		\
	 * integer that includes our byte, and load its value.		\
	 */								\
	ptr32 = (volatile u32 *)((unsigned long)ptr & ~0x3);		\
	load32 = *ptr32;						\
									\
	while (true) {							\
		/*							\
		 * Ensure the subword we want to exchange matches 	\
		 * the expected old value, and if not then bail.	\
		 */							\
		load = (load32 & mask) >> shift;			\
		if (load != old)					\
			return load;					\
									\
		/*							\
		 * Calculate the old & new values of the naturally	\
		 * aligned 4 byte integer including the byte we want	\
		 * to exchange. Attempt to exchange the old value	\
		 * for the new value, and return if we succeed.		\
		 */							\
		old32 = (load32 & ~mask) | (old << shift);		\
		new32 = (load32 & ~mask) | (new << shift);		\
		load32 = __op(ptr32, old32, new32);			\
		if (load32 == old32)					\
			return old;					\
	}								\
}

TEMPLATE_CMPXCGH_SMALL(__cmpxchg_small,cmpxchg)
TEMPLATE_CMPXCGH_SMALL(__cmpxchg_relaxed_small,cmpxchg_relaxed)
TEMPLATE_CMPXCGH_SMALL(__cmpxchg_acquire_small,cmpxchg_acquire)
TEMPLATE_CMPXCGH_SMALL(__cmpxchg_release_small,cmpxchg_release)
