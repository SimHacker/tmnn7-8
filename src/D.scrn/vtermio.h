/* vtermio.h -- low level screen I/O handling */

/* vtermio.c defines the standard interface manager input functions, plus: */

extern void vterminit();
extern int vputc();
extern void vflush();
extern bool vcheckin();

/* vtermio.h ends here */
