/* infinoted-plugin-replacer - An replacer infinoted plugin
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
//#include <libinftext/inf-text-session.h>
#include <libinftext/inf-text-buffer.h>
#include <math.h>

/* The plugin instance structure */
typedef struct _InfinotedPluginReplacer InfinotedPluginReplacer;
struct _InfinotedPluginReplacer {
  /* Pointer to the plugin manager, providing an interface to the
   * state of the server. */
  InfinotedPluginManager* manager;
  /* Greeting text for all new connections */
  gchar* greeting_text;
};
typedef struct _InfinotedPluginReplacerSession InfinotedPluginReplacerSession;
struct _InfinotedPluginReplacerSession{
  InfTextBuffer* buffer;
  InfSession* session;
};

static void
infinoted_plugin_replacer_info_initialize(gpointer plugin_info)
{
  InfinotedPluginReplacer* plugin;
  plugin = (InfinotedPluginReplacer*)plugin_info;

  plugin->manager = NULL;
  plugin->greeting_text = g_strdup("Hello"); /* Default value */
}

static gboolean
infinoted_plugin_replacer_initialize(InfinotedPluginManager* manager,
                                    gpointer plugin_info,
                                    GError** error)
{
  InfinotedPluginReplacer* plugin;
  gboolean result;

  plugin = (InfinotedPluginReplacer*)plugin_info;

  /* Pointer to plugin manager should be saved since it provides access
   * to the rest of infinoted. */
  plugin->manager = manager;

  /* Validate input option */
  if(plugin->greeting_text == NULL || *plugin->greeting_text == '\0')
  {
    g_set_error(
      error,
      g_quark_from_static_string("INFINOTED_PLUGIN_REPLACER_ERROR"),
      0,
      "Greeting text must not be empty"
    );

    return FALSE;
  }

  return TRUE;
}

static void
infinoted_plugin_replacer_deinitialize(gpointer plugin_info)
{
  InfinotedPluginReplacer* plugin;
  plugin = (InfinotedPluginReplacer*)plugin_info;

  g_free(plugin->greeting_text);
}

static void qualcosa(InfTextBuffer *inftextbuffer,
					guint          arg1,
					InfTextChunk  *arg2,
					InfUser       *arg3,
					gpointer       user_data){
  /*InfinotedPluginReplacerSession* session_info = 
	(InfinotedPluginReplacerSession*) user_data;
  const gchar* text = g_malloc(5*sizeof(gchar));
  
  InfUserTable* user_table = inf_session_get_user_table(session_info->session);
  
  
  text = "ciao";
  inf_text_buffer_insert_text(session_info->buffer,
							  0,
							  text,
							  4,
							  4,
							  arg3);*/
}

static void
infinoted_plugin_replacer_greet_user(InfinotedPluginReplacer* plugin,
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
infinoted_plugin_replacer_available_user_added_cb(InfUserTable* user_table,
                                                 InfUser* user,
                                                 gpointer user_data)
{
	
  infinoted_plugin_replacer_greet_user(
    (InfinotedPluginReplacer*)user_data,
    user
  );
}

static void
infinoted_plugin_replacer_foreach_user_func(InfUser* user,
                                           gpointer user_data)
{
  /* This is called for every user, also disconnected ones. We only greet
   * available users. */
  if(inf_user_get_status(user) != INF_USER_UNAVAILABLE)
  {
    infinoted_plugin_replacer_greet_user(
      (InfinotedPluginReplacer*)user_data,
      user
    );
  }
}

static void
infinoted_plugin_replacer_session_added(const InfBrowserIter* iter,
                                       InfSessionProxy* proxy,
                                       gpointer plugin_info,
                                       gpointer session_info_mem)
{
  InfSession* session;
  InfUserTable* user_table;
  InfinotedPluginReplacerSession* session_info = 
	(InfinotedPluginReplacerSession*) session_info_mem;
  
  g_object_get(G_OBJECT(proxy), "session", &session, NULL);
  session_info->buffer = 
	(InfTextBuffer*) inf_session_get_buffer(session);
  session_info->session = session;
  user_table = inf_session_get_user_table(session);
  
  /* Greet every user that will join: */
  g_signal_connect(
    G_OBJECT(user_table),
    "add-available-user",
    G_CALLBACK(infinoted_plugin_replacer_available_user_added_cb),
    plugin_info
  );
  g_signal_connect(
    G_OBJECT(session_info->buffer),
    "text-insterted",
    G_CALLBACK(qualcosa),
    session_info
  );

  /* Greet every user that has already joined: */
  inf_user_table_foreach_user(
    user_table,
    infinoted_plugin_replacer_foreach_user_func,
    plugin_info
  );

  g_object_unref(session);
}

static void
infinoted_plugin_replacer_session_removed(const InfBrowserIter* iter,
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
    G_CALLBACK(infinoted_plugin_replacer_available_user_added_cb),
    plugin_info
  );

  g_object_unref(session);
}

static const InfinotedParameterInfo INFINOTED_PLUGIN_REPLACER_OPTIONS[] = {
  {
    "greeting-text",
    INFINOTED_PARAMETER_STRING,
    0, /* no flags, if parameter is not given default is used */
    G_STRUCT_OFFSET(InfinotedPluginReplacer, greeting_text),
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
  "replacer",
  "An replacer plugin that writes a greeting message into the log for "
  "every user that joins a session.",
  INFINOTED_PLUGIN_REPLACER_OPTIONS,
  sizeof(InfinotedPluginReplacer),
  0,
  sizeof(InfinotedPluginReplacerSession),
  "InfTextSession",
  infinoted_plugin_replacer_info_initialize,
  infinoted_plugin_replacer_initialize,
  infinoted_plugin_replacer_deinitialize,
  NULL,
  NULL,
  infinoted_plugin_replacer_session_added,
  infinoted_plugin_replacer_session_removed,
};

/* vim:set et sw=2 ts=2: */
