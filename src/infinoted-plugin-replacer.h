
static void infinoted_plugin_replacer_join_user(InfinotedPluginReplacerSessionInfo*);
static void infinoted_plugin_replacer_remove_user(InfinotedPluginReplacerSessionInfo*);

static void infinoted_plugin_replacer_check_enabled(InfinotedPluginReplacerSessionInfo* info);

static void
infinoted_plugin_replacer_text_inserted_cb(InfTextBuffer* buffer,
                                             guint pos,
                                             InfTextChunk* chunk,
                                             InfUser* user,
                                             gpointer user_data);


static void
infinoted_plugin_replacer_text_erased_cb(InfTextBuffer* buffer,
                                           guint pos,
                                           InfTextChunk* chunk,
                                           InfUser* user,
                                           gpointer user_data);
