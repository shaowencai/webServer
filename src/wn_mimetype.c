#include <string.h>
#include "webnet.h"

struct webnet_mime_entry
{
    const char* name;
    const char* type;
};

static const struct webnet_mime_entry mime_tables[] =
{
    { "any",	"application/binary" }, /* default. */
    { "html",	"text/html" },
    { "htm",	"text/html" },
    { "css",	"text/css" },
    { "txt",	"text/plain" },
    { "pdf",	"application/pdf" },
    { "gif",	"image/gif" },
    { "png",	"image/png" },
    { "jpeg",	"image/jpeg" },
    { "jpg",	"image/jpeg" },
    { "avi",	"video/x-msvideo" },
    { "mp3",	"audio/mpeg" },
    { "ogg",	"audio/x-oggvorbis" },
    { "wav",	"audio/x-wav" },
    { "class",	"application/octet-stream" },
    { "js",		"application/x-javascript" },
    { "tar",	"application/x-tar" },
    { "zip",	"application/zip" },
    { "xml",	"text/xml" },
    { NULL,  NULL }
};

/***********************************
 * get mime type according to URL
 ***********************************/
const char* mime_get_type(const char* url)
{
    unsigned int index;

    index = 0;
    if (url == NULL)
    {
        return mime_tables[0].type;
    }

    while (mime_tables[index].name != NULL)
    {
        if (str_end_with(url, mime_tables[index].name))
        {
            return mime_tables[index].type;
        }

        index++;
    }

    /* return text/html as default */
    return mime_tables[0].type;
}
