#include	<stdarg.h>
#include	<stdlib.h>
#include    <string.h>
#include    <stdio.h>
#include <wn_malloc.h>
/********************************* Defines ************************************/
#define ROUNDUP4(size) ((size) % 4) ? (size) + (4 - ((size) % 4)) : (size)

/********************************** Locals ************************************/
static bType			*bQhead[B_MAX_CLASS];	/* Per class block q head */
static char				*bFreeBuf;				/* Pointer to free memory */
static char				*bFreeNext;				/* Pointer to next free mem */
static int				bFreeSize;				/* Size of free memory */
static int				bFreeLeft;				/* Size of free left for use */
static int				bFlags = B_USE_MALLOC;	/* Default to auto-malloc */
static int				bopenCount = 0;			/* Num tasks using balloc */

/********************************** Code **************************************/

static int ballocGetSize(int size, int *q)
{
    int mask;

    mask = (size == 0) ? 0 : (size-1) >> B_SHIFT;
    for (*q = 0; mask; mask >>= 1)
    {
        *q = *q + 1;
    }
    return ((1 << (B_SHIFT + *q)) + sizeof(bType));
}


int wn_open(void *buf, int bufsize, int flags)
{
	bFlags = flags;

	if (++bopenCount > 1) 
	{
		return 0;
	}

	if (buf == NULL) 
	{
		if (bufsize == 0) 
		{
			bufsize = B_DEFAULT_MEM;
		}
		if ((buf = malloc(bufsize)) == NULL) 
		{
			--bopenCount;
			return -1;
		}
	}
	else 
	{
	    //使用静态buf
		bFlags |= B_USER_BUF;
	}

	//bFreeSize是buf的总大小   bFreeLeft是剩余的空闲buf大小
	bFreeSize = bFreeLeft = bufsize;
	bFreeBuf = bFreeNext = buf;
	memset(bQhead, 0, sizeof(bQhead));
	return 0;
}


void wn_close()
{
	if (--bopenCount <= 0 && !(bFlags & B_USER_BUF)) 
	{
		free(bFreeBuf);
		bopenCount = 0;
	}
}

void *wn_malloc(int size)
{
	bType	*bp;
	int		q, memSize;

	if (bFreeBuf == NULL) 
	{
		if (wn_open(NULL, B_DEFAULT_MEM, 0) < 0)
		{
			return NULL;
		}
	}

	if (size < 0)
	{
		return NULL;
	}

	//Block classes are: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192,
	//进行字节对齐  最小的内存大小是16字节
	memSize = ballocGetSize(size, &q);

	//最大申请的内存是2^13
	if (q >= B_MAX_CLASS) 
	{
	    //超过最大申请的内存
		if (bFlags & B_USE_MALLOC) 
		{
		    //如果采用动态内存申请，就根据实际再申请一个大空间
			bp = (bType*) malloc(memSize);
			if (bp == NULL) 
			{
				printf("B: malloc failed\n");
				return NULL;
			}
		} 
		else 
		{
			return NULL;
		}

		//大小等于申请的字节数-头部
		bp->u.size = memSize - sizeof(bType);
		bp->flags = B_MALLOCED;
	} 
	else if ((bp = bQhead[q]) != NULL) 
	{
		bQhead[q] = bp->u.next;
		bp->u.size = memSize - sizeof(bType);
		bp->flags = 0;
	} 
	else 
	{
		if (bFreeLeft > memSize) 
		{
			bp = (bType*) bFreeNext;
			
			//浠庝箣鍓嶇殑澶у潡涓埅鍙栨寚瀹氬ぇ灏忕殑绌洪棿涓嬫潵
			bFreeNext += memSize;
			bFreeLeft -= memSize;
			bp->u.size = memSize - sizeof(bType);
			bp->flags = 0;
			
		} 
		else if (bFlags & B_USE_MALLOC) 
		{

			if ((bp = (bType*) malloc(memSize)) == NULL) 
			{
			    printf("B: malloc failed\n");
				return NULL;
			}
			bp->u.size = memSize - sizeof(bType);
			bp->flags = B_MALLOCED;

		} 
		else 
		{
		    printf("B: malloc failed\n");
			return NULL;
		}
	}

	bp->flags |= B_INTEGRITY;
	return (void*) ((char*) bp + sizeof(bType));
}


void wn_free(void *mp)
{
	bType	*bp;
	int		q = 0;

	if (!mp)
    {
        return;
    }
	//得到真正申请空间的起始地址
	bp = (bType*) ((char*) mp - sizeof(bType));

	if((bp->flags & B_INTEGRITY_MASK) != B_INTEGRITY)
	{
	    return;
	}

	if ((bp->flags & B_INTEGRITY_MASK) != B_INTEGRITY) 
	{
		return;
	}

	ballocGetSize(bp->u.size, &q);

	if (bp->flags & B_MALLOCED) 
	{
	    //如果是额外申请的，需要进行释放
		free(bp);
		return;
	}
	//将释放的空间挂在指定的链表下
	bp->u.next = bQhead[q];
	bQhead[q] = bp;
	bp->flags = B_FILL_WORD;
}

char *wn_strdup(const char *s)
{
	char	*cp;
	int		len;

	if (s == NULL) 
	{
		s = "";
	}
	len = strlen(s) + 1;
	if ((cp = wn_malloc(len * sizeof(char))) != NULL)
	{
		strcpy(cp, s);
	}
	return cp;
}

char *wn_memdup(char *s, int size)
{
	char	*cp;

    if ((cp = wn_malloc(size * sizeof(char))) != NULL)
    {
        if (s == NULL) 
        {
            memset(cp, 0, size);
        } 
        else 
        {
            memcpy(cp, s, size);
        }
	}
	return cp;
}


void *wn_realloc(void *mp, int newsize)
{
	bType	*bp;
	void	*newbuf;

	if (mp == NULL) 
	{
		return wn_malloc(newsize);
	}
	
	bp = (bType*) ((char*) mp - sizeof(bType));
	
	if((bp->flags & B_INTEGRITY_MASK) != B_INTEGRITY)
	{
	    return NULL;
	}

	if (bp->u.size >= newsize) 
	{
		return mp;
	}
	if ((newbuf = wn_malloc(newsize)) != NULL)
	{
		memcpy(newbuf, mp, bp->u.size);
		wn_free(mp);
	}
	return newbuf;
}
