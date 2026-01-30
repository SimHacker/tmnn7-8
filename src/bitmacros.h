/* bitmacros.h --  bitmap-allocation and get/set macros */

#define ALLOC_BITS(n)		calloc((unsigned) ALLOC_LEN(n), 1)
#define REALLOC_BITS(p, m, n)	if (m>n)(void)realloc(p,(unsigned)ALLOC_LEN(n))

#ifndef UNPACKED
#define ALLOC_LEN(n)		(((n) >> 3) + 1)
#define GET_BIT(map, b)		((map)[(b) >> 3] &   (1 << ((b) % 8)))
#define SET_BIT(map, b)		((map)[(b) >> 3] |=  (1 << ((b) % 8)))
#define CLEAR_BIT(map, b)	((map)[(b) >> 3] &= ~(1 << ((b) % 8)))
#else
#define ALLOC_LEN(n)		(n)
#define GET_BIT(map, b)		map[b]
#define SET_BIT(map, b)		(map[b] = 1)
#define CLEAR_BIT(map, b)	(map[b] = 0)
#endif /* !UNPACKED */

/* bitmacros.h ends here */
