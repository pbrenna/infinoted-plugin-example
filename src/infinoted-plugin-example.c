/* infinoted-plugin-example - An example infinoted plugin
 * Copyright (c) 2014, Armin Burgmeier <armin@arbur.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <infinoted/infinoted-plugin-manager.h>
#include <infinoted/infinoted-parameter.h>

/* The plugin instance structure */
typedef struct _InfinotedPluginExample InfinotedPluginExample;
struct _InfinotedPluginExample {
  /* Pointer to the plugin manager, providing an interface to the
   * state of the server. */
  InfinotedPluginManager* manager;
  /* Greeting text for all new connections */
  gchar* greeting_text;
};

static void
infinoted_plugin_example_info_initialize(gpointer plugin_info)
{
  InfinotedPluginExample* plugin;
  plugin = (InfinotedPluginExample*)plugin_info;

  plugin->manager = NULL;
  plugin->greeting_text = g_strdup("Hello"); /* Default value */
}

static gboolean
infinoted_plugin_example_initialize(InfinotedPluginManager* manager,
                                    gpointer plugin_info,
                                    GError** error)
{
  InfinotedPluginExample* plugin;
  gboolean result;

  plugin = (InfinotedPluginExample*)plugin_info;

  /* Pointer to plugin manager should be saved since it provides access
   * to the rest of infinoted. */
  plugin->manager = manager;

  /* Validate input option */
  if(plugin->greeting_text == NULL || *plugin->greeting_text == '\0')
  {
    g_set_error(
      error,
      g_quark_from_static_string("INFINOTED_PLUGIN_EXAMPLE_ERROR"),
      0,
      "Greeting text must not be empty"
    );

    return FALSE;
  }

  return TRUE;
}

static void
infinoted_plugin_example_deinitialize(gpointer plugin_info)
{
  InfinotedPluginExample* plugin;
  plugin = (InfinotedPluginExample*)plugin_info;

  g_free(plugin->greeting_text);
}

static void
infinoted_plugin_example_greet_user(InfinotedPluginExample* plugin,
                                    InfUser* user)
{
  InfinotedLog* log;
  log = infinoted_plugin_manager_get_log(plugin->manager);

  infinoted_log_info(
    log,
    "%s, %s",
    plugin->greeting_text,
    inf_user_get_name(user)
  );
}

static void
infinoted_plugin_example_available_user_added_cb(InfUserTable* user_table,
                                                 InfUser* user,
                                                 gpointer user_data)
{
  infinoted_plugin_example_greet_user(
    (InfinotedPluginExample*)user_data,
    user
  );
}

static void
infinoted_plugin_example_foreach_user_func(InfUser* user,
                                           gpointer user_data)
{
  /* This is called for every user, also disconnected ones. We only greet
   * available users. */
  if(inf_user_get_status(user) != INF_USER_UNAVAILABLE)
  {
    infinoted_plugin_example_greet_user(
      (InfinotedPluginExample*)user_data,
      user
    );
  }
}

static void
infinoted_plugin_example_session_added(const InfBrowserIter* iter,
                                       InfSessionProxy* proxy,
                                       gpointer plugin_info,
                                       gpointer session_info)
{
  InfSession* session;
  InfUserTable* user_table;

  g_object_get(G_OBJECT(proxy), "session", &session, NULL);
  user_table = inf_session_get_user_table(session);

  /* Greet every user that will join: */
  g_signal_connect(
    G_OBJECT(user_table),
    "add-available-user",
    G_CALLBACK(infinoted_plugin_example_available_user_added_cb),
    plugin_info
  );

  /* Greet every user that has already joined: */
  inf_user_table_foreach_user(
    user_table,
    infinoted_plugin_example_foreach_user_func,
    plugin_info
  );

  g_object_unref(session);
}

static void
infinoted_plugin_example_session_removed(const InfBrowserIter* iter,
                                         InfSessionProxy* proxy,
                                         gpointer plugin_info,
                                         gpointer session_info)
{
  InfSession* session;
  InfUserTable* user_table;

  g_object_get(G_OBJECT(proxy), "session", &session, NULL);
  user_table = inf_session_get_user_table(session);

  g_signal_handlers_disconnect_by_func(
    G_OBJECT(user_table),
    G_CALLBACK(infinoted_plugin_example_available_user_added_cb),
    plugin_info
  );

  g_object_unref(session);
}

static const InfinotedParameterInfo INFINOTED_PLUGIN_EXAMPLE_OPTIONS[] = {
  {
    "greeting-text",
    INFINOTED_PARAMETER_STRING,
    0, /* no flags, if parameter is not given default is used */
    G_STRUCT_OFFSET(InfinotedPluginExample, greeting_text),
    infinoted_parameter_convert_string,
    0,
    "Text that is written into the log for each user",
    "TEXT"
  }, {
    NULL,
    0,
    0,
    0,
    NULL
  }
};

const InfinotedPlugin INFINOTED_PLUGIN = {
  "example",
  "An example plugin that writes a greeting message into the log for "
  "every user that joins a session.",
  INFINOTED_PLUGIN_EXAMPLE_OPTIONS,
  sizeof(InfinotedPluginExample),
  0,
  0,
  NULL,
  infinoted_plugin_example_info_initialize,
  infinoted_plugin_example_initialize,
  infinoted_plugin_example_deinitialize,
  NULL,
  NULL,
  infinoted_plugin_example_session_added,
  infinoted_plugin_example_session_removed,
};

/* vim:set et sw=2 ts=2: */
