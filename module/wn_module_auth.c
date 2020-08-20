#include <string.h>
#include <webnet.h>

#if WEBNET_USING_AUTH

struct webnet_auth_item
{
    char *path;

    /* username and password, which will encode as base64 as - username:password*/
    char *username_password;
};

static struct webnet_auth_item* _auth_items = NULL;
static uint32_t _auth_items_count = 0;

/**
 * set the authorization on the path
 *
 * @param path the path to be authorized
 * @param username_password the user and password, which format shall be
 * username:password
 */
void webnet_auth_set(const char* path, const char* username_password)
{
    if (_auth_items == NULL)
    {
        _auth_items_count = 1;
        _auth_items = (struct webnet_auth_item*)wn_malloc (sizeof(struct webnet_auth_item) *
                      _auth_items_count);
    }
    else
    {
        unsigned long index;

        /* check whether modify a password */
        for (index = 0; index < _auth_items_count; index ++)
        {
            if (strcmp(path, _auth_items[index].path) == 0)
            {
                wn_free(_auth_items[index].username_password);
                _auth_items[index].username_password = str_base64_encode(username_password);
                return;
            }
        }

        _auth_items_count += 1;
        _auth_items = (struct webnet_auth_item*) wn_realloc (_auth_items, sizeof(struct webnet_auth_item) *
                      _auth_items_count);
    }

    ASSERT(_auth_items != NULL);

    _auth_items[_auth_items_count - 1].path = wn_strdup(path);
    _auth_items[_auth_items_count - 1].username_password = str_base64_encode(username_password);
}

/**
 * Authorization module handler
 */
int webnet_module_auth(struct webnet_session* session)
{
    uint32_t index;
    struct webnet_request *request;

    ASSERT(session != NULL);
    request = session->request;
    ASSERT(request != NULL);

    /* check authorization item */
    for (index = 0; index < _auth_items_count; index ++)
    {
        if (str_path_with(request->path, _auth_items[index].path))
        {
            if (request->authorization == NULL ||
                    strlen(_auth_items[index].username_password) !=
                    strlen(request->authorization))
            {
                /* set authorization request, 401 */
                request->result_code = 401;
                return WEBNET_MODULE_FINISHED;
            }

            /* check authorization */
            if (strcmp(request->authorization,
                       _auth_items[index].username_password) == 0)
            {
                /* authorization OK */
                request->result_code = 200;
                return WEBNET_MODULE_CONTINUE;
            }
            else
            {
                /* set authorization request, 401 */
                request->result_code = 401;
                return WEBNET_MODULE_FINISHED;
            }
        }
    }

    return WEBNET_MODULE_CONTINUE;
}

#endif /* WEBNET_USING_AUTH */
