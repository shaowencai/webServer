#include <webnet.h>

#if WEBNET_USING_ALIAS

struct webnet_alias_item
{
    char* old_path;
    char* new_path;
};
static struct webnet_alias_item *_alias_items = NULL;
static uint32_t _alias_item_count = 0;

void webnet_alias_set(char* old_path, char* new_path)
{
    if (_alias_items == NULL)
    {
        _alias_item_count = 1;
        _alias_items = (struct webnet_alias_item*)wn_malloc (sizeof(struct webnet_alias_item) *
                       _alias_item_count);
    }
    else
    {
        _alias_item_count += 1;
        _alias_items = (struct webnet_alias_item*) wn_realloc (_alias_items, sizeof(struct webnet_alias_item) *
                       _alias_item_count);
    }

    ASSERT(_alias_items != NULL);

    _alias_items[_alias_item_count - 1].old_path = wn_strdup(old_path);
    ASSERT(_alias_items[_alias_item_count - 1].old_path != NULL);
    _alias_items[_alias_item_count - 1].new_path = wn_strdup(new_path);
    ASSERT(_alias_items[_alias_item_count - 1].new_path != NULL);

    return;
}

int webnet_module_alias(struct webnet_session* session)
{
    int index;
    struct webnet_request* request;

    ASSERT(session != NULL);
    request = session->request;
    ASSERT(request != NULL);

    /* check whether the uri is a alias */
    for (index = 0; index < _alias_item_count; index ++)
    {
        if (str_path_with(request->path, _alias_items[index].old_path))
        {
            char* map_path;
            map_path = (char*) wn_malloc (WEBNET_PATH_MAX);
            ASSERT(map_path != NULL);

            snprintf(map_path, WEBNET_PATH_MAX, "%s/%s",
                        _alias_items[index].new_path,
                        request->path + strlen(_alias_items[index].old_path));

            /* set new path */
            wn_free(request->path);
            request->path = map_path;

            return WEBNET_MODULE_CONTINUE;
        }
    }

    return WEBNET_MODULE_CONTINUE;
}

#endif /* WEBNET_USING_ALIAS */
