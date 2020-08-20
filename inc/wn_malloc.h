
#ifndef __WN_MALLOC_H__
#define __WN_MALLOC_H__

typedef struct {
    union {
        void    *next;                          /* Pointer to next in q */
        int     size;                           /* Actual requested size */
    } u;
    int         flags;                          /* Per block allocation flags */
} bType;


#define B_USE_MALLOC    0x1                     /* Okay to use malloc if required */
#define B_USER_BUF      0x2                     /* User supplied buffer for mem */
#define B_MAX_CLASS     13                      /* Maximum class number + 1 */
#define B_SHIFT         4                       /* Convert size to class */
#define B_MALLOCED      0x80000000              /* Block was malloced */
#define B_FILL_WORD     0x77777777              /* Fill word for buffers */
#define B_INTEGRITY      0x8124000              /* Integrity value */
#define B_INTEGRITY_MASK 0xFFFF000              /* Integrity mask */
#define B_DEFAULT_MEM   (64 * 1024)             /* Default memory allocation */

void*   wn_realloc(void *mp, int newsize);
char*   wn_memdup(char *s, int size);
char*   wn_strdup(const char *s);
void    wn_free(void *mp);
void*   wn_malloc(int size);
void    wn_close();
int     wn_open(void *buf, int bufsize, int flags);

#endif
