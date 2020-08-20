#include <string.h>
#include <webnet.h>


#if WEBNET_USING_CGI

#define CGI_ROOT_PATH_MAX	64

static char _cgi_root[CGI_ROOT_PATH_MAX] = "/cgi-bin/";

struct webnet_cgi_group
{
    struct webnet_cgi_item* _cgi_items;
    uint32_t _cgi_count;
};

static int (*before_handle_hook)(struct webnet_session* session,struct webnet_cgi_item*);
static int (*after_handle_hook)(struct webnet_session* session,struct webnet_cgi_item*);

static struct webnet_cgi_group cgi_groups[13]={0};

static int hashIndex(const char *name)
{
    unsigned int    sum = 0;
    int             i = 0;

    while (*name)
    {
        sum += (((int) *name++) << i);
        i = (i + 7) % ( 8*sizeof(int) - 8);
    }
    return sum % 13;
}


void webnet_cgi_set_root(const char* root)
{
    if (strlen(root) > CGI_ROOT_PATH_MAX)
    {
        return;
    }

    snprintf(_cgi_root,sizeof(_cgi_root), root);
    
    if (_cgi_root[strlen(_cgi_root)] != '/')
    {
        _cgi_root[strlen(_cgi_root) + 1] = '/';
        _cgi_root[strlen(_cgi_root) + 1] = '\0';
    }
}

void webnet_cgi_set_hook_before_handle(int (*fun)(void*,void*))
{
    before_handle_hook = (int (*)(struct webnet_session* session,struct webnet_cgi_item*))fun;
}

void webnet_cgi_set_hook_after_handle(int (*fun)(void*,void*))
{
    after_handle_hook = (int (*)(struct webnet_session* session,struct webnet_cgi_item*))fun;
}

void webnet_cgi_register(const char* name,uint16_t level,void (*handler)(struct webnet_session* session))
{
    int groupIndex = 0;

    ASSERT(name != NULL);
    ASSERT(handler != NULL);

    groupIndex = hashIndex(name);

    if (cgi_groups[groupIndex]._cgi_items == NULL)
    {
        cgi_groups[groupIndex]._cgi_count = 1;
        cgi_groups[groupIndex]._cgi_items = (struct webnet_cgi_item*) wn_malloc (sizeof(struct webnet_cgi_item) *  cgi_groups[groupIndex]._cgi_count);
    }
    else
    {
        cgi_groups[groupIndex]._cgi_count += 1;
        cgi_groups[groupIndex]._cgi_items = (struct webnet_cgi_item*) wn_realloc (cgi_groups[groupIndex]._cgi_items, sizeof(struct webnet_cgi_item) * cgi_groups[groupIndex]._cgi_count);
    }

    ASSERT(cgi_groups[groupIndex]._cgi_items != NULL);
    cgi_groups[groupIndex]._cgi_items[cgi_groups[groupIndex]._cgi_count - 1].name = name;
    cgi_groups[groupIndex]._cgi_items[cgi_groups[groupIndex]._cgi_count - 1].level = level;
    cgi_groups[groupIndex]._cgi_items[cgi_groups[groupIndex]._cgi_count - 1].handler = handler;
}

int webnet_module_cgi(struct webnet_session* session)
{
    struct webnet_request* request;
    char *cgi_path = NULL;
    int groupIndex = 0;
    int result = 1;

    ASSERT(session != NULL);
    request = session->request;
    ASSERT(request != NULL);

    /* check whether a cgi request */
    cgi_path = strstr(request->path, _cgi_root);
    if (cgi_path != NULL)
    {
        char* cgi_name;
        uint32_t index;

        cgi_name = cgi_path + strlen(_cgi_root);
        groupIndex = hashIndex(cgi_name);

        for (index = 0; index < cgi_groups[groupIndex]._cgi_count; index ++)
        {
            if ((strlen(cgi_name) == strlen(cgi_groups[groupIndex]._cgi_items[index].name))
                    && strncasecmp(cgi_name, cgi_groups[groupIndex]._cgi_items[index].name, strlen(cgi_groups[groupIndex]._cgi_items[index].name)) == 0)
            {
                /* found it */
                if(before_handle_hook)
                {
                    result = before_handle_hook(session,&cgi_groups[groupIndex]._cgi_items[index]);
                }
                if(result)
                {
                    cgi_groups[groupIndex]._cgi_items[index].handler(session);
                }

                if(after_handle_hook)
                {
                    after_handle_hook(session,&cgi_groups[groupIndex]._cgi_items[index]);
                }
                return WEBNET_MODULE_FINISHED;
            }
        }

        /* set 404 not found error */
        request->result_code = 404;
    }

    return WEBNET_MODULE_CONTINUE;
}

#endif /* WEBNET_USING_CGI */
