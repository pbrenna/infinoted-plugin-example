/*
 * infinoted-plugin-replacer - a real-time replacer for text sessions
 * on an infinoted server.
 * Copyright (C) 2016 Pietro Brenna <pietrobrenna@zoho.com>
 * Copyright (C) 2016 Alessandro Bregoli <>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301, USA.
*/
/* libinfinity - a GObject-based infinote implementation
 * Copyright (C) 2007-2015 Armin Burgmeier <armin@arbur.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <infinoted/infinoted-plugin-manager.h>
#include <infinoted/infinoted-parameter.h>

#include <libinftext/inf-text-session.h>
#include <libinftext/inf-text-buffer.h>

#include <libinfinity/common/inf-request-result.h>
#include "inf-signals.h"
//#include "inf-i18n.h"
#include <string.h>

#define INFINOTED_PLUGIN_REPLACER_KEY_GROUP "replacer"
typedef struct _InfinotedPluginReplacer InfinotedPluginReplacer;
struct _InfinotedPluginReplacer {
  InfinotedPluginManager* manager;
  gchar* replace_table;
  gchar** replace_words;
  GKeyFile* replace_dict;
  gsize replace_words_len;
};

typedef struct _InfinotedPluginReplacerSessionInfo
  InfinotedPluginReplacerSessionInfo;
struct _InfinotedPluginReplacerSessionInfo {
  InfinotedPluginReplacer* plugin;
  InfSessionProxy* proxy;
  InfRequest* request;
  InfUser* user;
  InfTextBuffer* buffer;
  InfIoDispatch* dispatch;
  gboolean enabled;
  gboolean locked;
};

typedef struct _InfinotedPluginReplacerHasAvailableUsersData
  InfinotedPluginReplacerHasAvailableUsersData;
struct _InfinotedPluginReplacerHasAvailableUsersData {
  InfUser* own_user;
  gboolean has_available_user;
};

#include "infinoted-plugin-replacer.h"

static void 
infinoted_plugin_replacer_info_initialize(gpointer plugin_info){
  InfinotedPluginReplacer* plugin;
  plugin = (InfinotedPluginReplacer*)plugin_info;
  plugin->replace_table = g_strdup("Aaaaaaaasd");
}


static void 
infinoted_plugin_replacer_clean_key(gchar* key){
	//replace final _ with space
	guint key_slen = strlen(key);
	if (key[key_slen - 1] == '_'){
		key[key_slen - 1] = ' ';
	}
}

static gboolean
infinoted_plugin_replacer_initialize(InfinotedPluginManager* manager,
                                       gpointer plugin_info,
                                       GError** error)
{
  InfinotedPluginReplacer* plugin;
  plugin = (InfinotedPluginReplacer*)plugin_info;

  plugin->manager = manager;
	GKeyFile* gkf = g_key_file_new();
	gboolean success = g_key_file_load_from_file(gkf, plugin->replace_table, G_KEY_FILE_NONE, error);
	if (FALSE == success)
		return FALSE;
	plugin->replace_dict = gkf;
	plugin->replace_words = g_key_file_get_keys (gkf,
																							 INFINOTED_PLUGIN_REPLACER_KEY_GROUP,
																							 &plugin->replace_words_len,
																							 error);
	if (NULL == plugin->replace_words)
		return FALSE;
	
	//foreach replace_words
	for (guint i = 0; i < plugin->replace_words_len; i++) {
		//check no recursion:
		//get value
		gchar* key = g_strdup(plugin->replace_words[i]);
		gchar* val = g_key_file_get_value(gkf, INFINOTED_PLUGIN_REPLACER_KEY_GROUP, key, error);
		if (NULL == val)
			return FALSE;
		infinoted_plugin_replacer_clean_key(key);
		if (NULL != strstr(val, key)) {
			g_set_error(error, 0, 1, "recursive key '%s' in group '%s'", key, INFINOTED_PLUGIN_REPLACER_KEY_GROUP);
			return FALSE;
		}
		g_free(key);
	}
	/*
	//read file
	gchar* table_file;	//la variabile viene allocata nella funzione g_file_get_contents
	gsize table_file_len;
	gboolean success = g_file_get_contents(plugin->replace_table,
																				 &table_file,
																				 &table_file_len,
																				 error);
	if (success == TRUE) {
		infinoted_plugin_replacer_parse_table(plugin);
	}
	g_free(table_file);*/
	return success;
}

static void
infinoted_plugin_replacer_deinitialize(gpointer plugin_info)
{
  InfinotedPluginReplacer* plugin;
  plugin = (InfinotedPluginReplacer*)plugin_info;
  g_strfreev(plugin->replace_words);
}


static void
infinoted_plugin_replacer_run(InfinotedPluginReplacerSessionInfo* info)
{
	//block text-insert and text-erase signal dispatch
	g_signal_handlers_block_by_func(
    info->buffer,
    G_CALLBACK(infinoted_plugin_replacer_text_inserted_cb),
    info
  );
  g_signal_handlers_block_by_func(
    info->buffer,
    G_CALLBACK(infinoted_plugin_replacer_text_erased_cb),
    info
  );
	infinoted_plugin_replacer_check_enabled( info);  
	if (FALSE == info->enabled)
		return;
  //guint substitutions = 0; //conto delle sostituzioni, per evitare
														 //ricorsioni disastrose
	InfTextBuffer* buf = info->buffer;


  //foreach replace_words
	for (guint i = 0; i < info->plugin->replace_words_len; i++) {
		gint diff = 0;
		//foreach word the buffer must be re-read to allow nested macros
		InfTextChunk* chunk = inf_text_buffer_get_slice(buf,
																										0,
																										inf_text_buffer_get_length(buf));
		gsize out_len;
		gchar* buf_str = inf_text_chunk_get_text(chunk, &out_len);
		gchar* tmp_buf_str = buf_str;
		//get key string
		gchar* key = g_strdup(info->plugin->replace_words[i]);	//g_strdup because it will be modified
		guint key_slen = strlen(key);
		guint key_ulen = g_utf8_strlen(key, -1);
		//get val string
		gchar* val = g_key_file_get_value(info->plugin->replace_dict, INFINOTED_PLUGIN_REPLACER_KEY_GROUP, key, NULL);
		if (val == NULL) continue;
		guint val_slen = strlen(val);
		guint val_ulen = g_utf8_strlen(val, -1);
		
		infinoted_plugin_replacer_clean_key(key);
		
		InfinotedLog* log = infinoted_plugin_manager_get_log(info->plugin->manager);
				infinoted_log_info(
					log,
					"key: %s", key);
		//get position, if present
		while (NULL != (tmp_buf_str = g_strstr_len(tmp_buf_str, -1, key))) {
			//g_strstr_len returns a pointer to the location of key
			//however we need the character count to the location
			glong offset = g_utf8_pointer_to_offset (buf_str, tmp_buf_str);
			offset += diff;
			diff += val_ulen - key_ulen;
			//replace
			inf_text_buffer_insert_text(buf, offset,val,val_slen, val_ulen, info->user);
			inf_text_buffer_erase_text(buf, offset + val_ulen, key_ulen, info->user);
			tmp_buf_str += strlen(key);
		}
		g_free(key);
		
	}
	g_signal_handlers_unblock_by_func(
    info->buffer,
    G_CALLBACK(infinoted_plugin_replacer_text_inserted_cb),
    info
  );
  g_signal_handlers_unblock_by_func(
    info->buffer,
    G_CALLBACK(infinoted_plugin_replacer_text_erased_cb),
    info
  );
}

static void
infinoted_plugin_replacer_run_dispatch_func(gpointer user_data)
{
  InfinotedPluginReplacerSessionInfo* info;
  info = (InfinotedPluginReplacerSessionInfo*)user_data;

  info->dispatch = NULL;

  infinoted_plugin_replacer_run(info);
}

static void
infinoted_plugin_replacer_check_enabled(InfinotedPluginReplacerSessionInfo* info)
{
	InfTextBuffer* buffer = info->buffer;
	//Magic string to use at the beginning of the file
  gchar* magic_string = "#replacer on\n";
  guint magic_string_length = strlen(magic_string);
  guint buffer_length = inf_text_buffer_get_length(buffer);
  if (buffer_length > magic_string_length){
		InfTextChunk* inizio = inf_text_buffer_get_slice(buffer, 0, magic_string_length);
		gsize real_len = inf_text_chunk_get_length(inizio);
		gchar* inizio_chars = inf_text_chunk_get_text(inizio, &real_len);
		//inizio_chars non è nul_terminated come invece strcmp richiede
		gchar nul_terminated[magic_string_length+1];
		memcpy(nul_terminated, inizio_chars, magic_string_length);
		nul_terminated[magic_string_length]='\0';
				
		InfinotedLog* log = infinoted_plugin_manager_get_log(info->plugin->manager);
		if (0 == g_strcmp0(nul_terminated, magic_string)) {
			if (!info->enabled) {
				infinoted_log_info(
					log,
					"Replacer turned on by magic string; %s", info->plugin->replace_table);
				info->enabled = TRUE;
			}
		} else {
			if (info->enabled) {
				infinoted_log_info(
					log,
					"Replacer turned off: %s", nul_terminated);
				info->enabled = FALSE;
			}
		}
	}
}


static void
infinoted_plugin_replacer_text_inserted_cb(InfTextBuffer* buffer,
                                             guint pos,
                                             InfTextChunk* chunk,
                                             InfUser* user,
                                             gpointer user_data)
{
  InfinotedPluginReplacerSessionInfo* info;
  info = (InfinotedPluginReplacerSessionInfo*)user_data;
  

  //check enabled
  InfdDirectory* directory;
	
  if(info->dispatch == NULL)
  {
    directory = infinoted_plugin_manager_get_directory(info->plugin->manager);

    info->dispatch = inf_io_add_dispatch(
      infd_directory_get_io(directory),
      infinoted_plugin_replacer_run_dispatch_func,
      info,
      NULL
    );
  }
}

static void
infinoted_plugin_replacer_text_erased_cb(InfTextBuffer* buffer,
                                           guint pos,
                                           InfTextChunk* chunk,
                                           InfUser* user,
                                           gpointer user_data)
{
  InfinotedPluginReplacerSessionInfo* info;
  InfdDirectory* directory;

  info = (InfinotedPluginReplacerSessionInfo*)user_data;

  
  if(info->dispatch == NULL)
  {
    directory = infinoted_plugin_manager_get_directory(info->plugin->manager);

    info->dispatch = inf_io_add_dispatch(
      infd_directory_get_io(directory),
      infinoted_plugin_replacer_run_dispatch_func,
      info,
      NULL
    );
  }
}

static void
infinoted_plugin_replacer_remove_user(
  InfinotedPluginReplacerSessionInfo* info)
{
  InfSession* session;
  InfUser* user;

  g_assert(info->user != NULL);
  g_assert(info->request == NULL);

  user = info->user;
  info->user = NULL;

  g_object_get(G_OBJECT(info->proxy), "session", &session, NULL); 

  inf_session_set_user_status(session, user, INF_USER_UNAVAILABLE);
  g_object_unref(user);

  inf_signal_handlers_disconnect_by_func(
    G_OBJECT(info->buffer),
    G_CALLBACK(infinoted_plugin_replacer_text_inserted_cb),
    info
  );

  inf_signal_handlers_disconnect_by_func(
    G_OBJECT(info->buffer),
    G_CALLBACK(infinoted_plugin_replacer_text_erased_cb),
    info
  );

  g_object_unref(session);
}

static void
infinoted_plugin_replacer_has_available_users_foreach_func(InfUser* user,
                                                             gpointer udata)
{
  InfinotedPluginReplacerHasAvailableUsersData* data;
  data = (InfinotedPluginReplacerHasAvailableUsersData*)udata;

  /* Return TRUE if there are non-local users connected */
  if(user != data->own_user &&
     inf_user_get_status(user) != INF_USER_UNAVAILABLE &&
     (inf_user_get_flags(user) & INF_USER_LOCAL) == 0)
  {
    data->has_available_user = TRUE;
  }
}

static gboolean
infinoted_plugin_replacer_has_available_users(
  InfinotedPluginReplacerSessionInfo* info)
{
  InfinotedPluginReplacerHasAvailableUsersData data;
  InfSession* session;
  InfUserTable* user_table;

  g_object_get(G_OBJECT(info->proxy), "session", &session, NULL); 
  user_table = inf_session_get_user_table(session);

  data.has_available_user = FALSE;
  data.own_user = info->user;

  inf_user_table_foreach_user(
    user_table,
    infinoted_plugin_replacer_has_available_users_foreach_func,
    &data
  );

  g_object_unref(session);
  return data.has_available_user;
}

static void
infinoted_plugin_replacer_user_join_cb(InfRequest* request,
                                         const InfRequestResult* result,
                                         const GError* error,
                                         gpointer user_data)
{
  InfinotedPluginReplacerSessionInfo* info;
  InfUser* user;

  info = (InfinotedPluginReplacerSessionInfo*)user_data;

  info->request = NULL;

  if(error != NULL)
  {
    infinoted_log_warning(
      infinoted_plugin_manager_get_log(info->plugin->manager),
      "Could not join Replacer user for document: %s\n",
      error->message
    );
  }
  else
  {
    inf_request_result_get_join_user(result, NULL, &user);

    info->user = user;
    g_object_ref(info->user);

    /* Initial run */
    infinoted_plugin_replacer_run(info);

    g_signal_connect(
      G_OBJECT(info->buffer),
      "text-inserted",
      G_CALLBACK(infinoted_plugin_replacer_text_inserted_cb),
      info
    );

    g_signal_connect(
      G_OBJECT(info->buffer),
      "text-erased",
      G_CALLBACK(infinoted_plugin_replacer_text_erased_cb),
      info
    );

    /* It can happen that while the request is being processed, the situation
     * changes again. */
    if(infinoted_plugin_replacer_has_available_users(info) == FALSE)
    {
      infinoted_plugin_replacer_remove_user(info);
    }
  }
}

static void
infinoted_plugin_replacer_add_available_user_cb(InfUserTable* user_table,
                                                  InfUser* user,
                                                  gpointer user_data);

static void
infinoted_plugin_replacer_join_user(
  InfinotedPluginReplacerSessionInfo* info)
{
  InfSession* session;
  InfUserTable* user_table;

  g_assert(info->user == NULL);
  g_assert(info->request == NULL);

  g_object_get(G_OBJECT(info->proxy), "session", &session, NULL);
  user_table = inf_session_get_user_table(session);

  /* Prevent double user join attempt by blocking the callback for
   * joining our local user. */
  g_signal_handlers_block_by_func(
    user_table,
    G_CALLBACK(infinoted_plugin_replacer_add_available_user_cb),
    info
  );

  info->request = inf_text_session_join_user(
    info->proxy,
    "Replacer",
    INF_USER_ACTIVE,
    0.0,
    inf_text_buffer_get_length(info->buffer),
    0,
    infinoted_plugin_replacer_user_join_cb,
    info
  );

  g_signal_handlers_unblock_by_func(
    user_table,
    G_CALLBACK(infinoted_plugin_replacer_add_available_user_cb),
    info
  );

  g_object_unref(session);
}

static void
infinoted_plugin_replacer_add_available_user_cb(InfUserTable* user_table,
                                                  InfUser* user,
                                                  gpointer user_data)
{
  InfinotedPluginReplacerSessionInfo* info;
  info = (InfinotedPluginReplacerSessionInfo*)user_data;

  if(info->user == NULL && info->request == NULL &&
     infinoted_plugin_replacer_has_available_users(info))
  {
    infinoted_plugin_replacer_join_user(info);
  }
}

static void
infinoted_plugin_replacer_remove_available_user_cb(InfUserTable* user_table,
                                                     InfUser* user,
                                                     gpointer user_data)
{
  InfinotedPluginReplacerSessionInfo* info;
  info = (InfinotedPluginReplacerSessionInfo*)user_data;

  if(info->user != NULL &&
     !infinoted_plugin_replacer_has_available_users(info))
  {
    infinoted_plugin_replacer_remove_user(info);
  }
}

static void
infinoted_plugin_replacer_session_added(const InfBrowserIter* iter,
                                          InfSessionProxy* proxy,
                                          gpointer plugin_info,
                                          gpointer session_info)
{
  InfinotedPluginReplacerSessionInfo* info;
  InfSession* session;
  InfUserTable* user_table;

  info = (InfinotedPluginReplacerSessionInfo*)session_info;

  info->plugin = (InfinotedPluginReplacer*)plugin_info;
  info->proxy = proxy;
  info->request = NULL;
  info->user = NULL;
  info->dispatch = NULL;
  info->enabled = FALSE;
  info->locked = FALSE;
  g_object_ref(proxy);

  g_object_get(G_OBJECT(proxy), "session", &session, NULL);
  g_assert(inf_session_get_status(session) == INF_SESSION_RUNNING);
  info->buffer = INF_TEXT_BUFFER(inf_session_get_buffer(session));
  g_object_ref(info->buffer);

  user_table = inf_session_get_user_table(session);

  g_signal_connect(
    G_OBJECT(user_table),
    "add-available-user",
    G_CALLBACK(infinoted_plugin_replacer_add_available_user_cb),
    info
  );

  g_signal_connect(
    G_OBJECT(user_table),
    "remove-available-user",
    G_CALLBACK(infinoted_plugin_replacer_remove_available_user_cb),
    info
  );

  /* Only join a user when there are other nonlocal users available, so that
   * we don't keep the session from going idle. */
  if(infinoted_plugin_replacer_has_available_users(info) == TRUE)
    infinoted_plugin_replacer_join_user(info);

  g_object_unref(session);
}

static void
infinoted_plugin_replacer_session_removed(const InfBrowserIter* iter,
                                            InfSessionProxy* proxy,
                                            gpointer plugin_info,
                                            gpointer session_info)
{
  InfinotedPluginReplacerSessionInfo* info;
  InfdDirectory* directory;
  InfSession* session;
  InfUserTable* user_table;

  info = (InfinotedPluginReplacerSessionInfo*)session_info;

  g_object_get(G_OBJECT(info->proxy), "session", &session, NULL);
  user_table = inf_session_get_user_table(session);

  g_signal_handlers_disconnect_by_func(
    G_OBJECT(user_table),
    G_CALLBACK(infinoted_plugin_replacer_add_available_user_cb),
    info
  );

  g_signal_handlers_disconnect_by_func(
    G_OBJECT(user_table),
    G_CALLBACK(infinoted_plugin_replacer_remove_available_user_cb),
    info
  );

  if(info->dispatch != NULL)
  {
    directory = infinoted_plugin_manager_get_directory(info->plugin->manager);
    inf_io_remove_dispatch(infd_directory_get_io(directory), info->dispatch);
    info->dispatch = NULL;
  }

  if(info->user != NULL)
  {
    infinoted_plugin_replacer_remove_user(info);
  }

  if(info->buffer != NULL)
  {
    g_object_unref(info->buffer);
    info->buffer = NULL;
  }

  if(info->request != NULL)
  {
    inf_signal_handlers_disconnect_by_func(
      info->request,
      G_CALLBACK(infinoted_plugin_replacer_user_join_cb),
      info
    );

    info->request = NULL;
  }

  g_assert(info->proxy != NULL);
  g_object_unref(info->proxy);

  g_object_unref(session);
}

static const InfinotedParameterInfo INFINOTED_PLUGIN_REPLACER_OPTIONS[] = {
  {
    "replace-table",
    INFINOTED_PARAMETER_STRING,
    INFINOTED_PARAMETER_REQUIRED,
    G_STRUCT_OFFSET(InfinotedPluginReplacer, replace_table),
    infinoted_parameter_convert_string,
    0,
    "File to be used as a replace table.",
    "RTABLE"
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
  "This plugin replaces text with custom snippets defined in a \
		configuration file",
  INFINOTED_PLUGIN_REPLACER_OPTIONS,
  sizeof(InfinotedPluginReplacer),
  0,
  sizeof(InfinotedPluginReplacerSessionInfo),
  "InfTextSession",
  infinoted_plugin_replacer_info_initialize,
  infinoted_plugin_replacer_initialize,
  infinoted_plugin_replacer_deinitialize,
  NULL,
  NULL,
  infinoted_plugin_replacer_session_added,
  infinoted_plugin_replacer_session_removed
};
