/* $Id: dfb-display.c,v 1.4 2010/06/05 14:28:17 fredette Exp $ */

/* host/dfb/dfb-display.c - DFB display support: */

/*
 * Copyright (c) 2003 Matt Fredette
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Matt Fredette.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <tme/common.h>
_TME_RCSID("$Id: dfb-display.c,v 1.4 2010/06/05 14:28:17 fredette Exp $");

/* includes: */
#include "dfb-display.h"

/* macros: */

/* the DFB display callout function.  it must be called with the mutex locked: */
void
_tme_dfb_display_callout(struct tme_dfb_display *display,
			 int new_callouts)
{
  struct tme_keyboard_connection *conn_keyboard;
  struct tme_mouse_connection *conn_mouse;
  int callouts, later_callouts;
  unsigned int ctrl;
  int rc;
  
  /* add in any new callouts: */
  display->tme_dfb_display_callout_flags |= new_callouts;

  /* if this function is already running in another thread, simply
     return now.  the other thread will do our work: */
  if (display->tme_dfb_display_callout_flags
      & TME_DFB_DISPLAY_CALLOUT_RUNNING) {
    return;
  }

  /* callouts are now running: */
  display->tme_dfb_display_callout_flags
    |= TME_DFB_DISPLAY_CALLOUT_RUNNING;

  /* assume that we won't need any later callouts: */
  later_callouts = 0;

  /* loop while callouts are needed: */
  for (; ((callouts
	   = display->tme_dfb_display_callout_flags)
	  & TME_DFB_DISPLAY_CALLOUTS_MASK); ) {

    /* clear the needed callouts: */
    display->tme_dfb_display_callout_flags
      = (callouts
	 & ~TME_DFB_DISPLAY_CALLOUTS_MASK);
    callouts
      &= TME_DFB_DISPLAY_CALLOUTS_MASK;

    /* get our keyboard connection: */
    conn_keyboard = display->tme_dfb_display_keyboard_connection;

    /* if we need to call out new keyboard control information: */
    if (callouts & TME_DFB_DISPLAY_CALLOUT_KEYBOARD_CTRL) {

      /* form the new ctrl: */
      ctrl = 0;
      if (!tme_keyboard_buffer_is_empty(display->tme_dfb_display_keyboard_buffer)) {
	ctrl |= TME_KEYBOARD_CTRL_OK_READ;
      }

      /* unlock the mutex: */
      tme_mutex_unlock(&display->tme_dfb_display_mutex);
      
      /* do the callout: */
      rc = (conn_keyboard != NULL
	    ? ((*conn_keyboard->tme_keyboard_connection_ctrl)
	       (conn_keyboard,
		ctrl))
	    : TME_OK);
	
      /* lock the mutex: */
      tme_mutex_lock(&display->tme_dfb_display_mutex);
      
      /* if the callout was unsuccessful, remember that at some later
	 time this callout should be attempted again: */
      if (rc != TME_OK) {
	later_callouts |= TME_DFB_DISPLAY_CALLOUT_KEYBOARD_CTRL;
      }
    }

    /* get our mouse connection: */
    conn_mouse = display->tme_dfb_display_mouse_connection;

    /* if we need to call out new mouse control information: */
    if (callouts & TME_DFB_DISPLAY_CALLOUT_MOUSE_CTRL) {

      /* form the new ctrl: */
      ctrl = 0;
      if (!tme_mouse_buffer_is_empty(display->tme_dfb_display_mouse_buffer)) {
	ctrl |= TME_MOUSE_CTRL_OK_READ;
      }

      /* unlock the mutex: */
      tme_mutex_unlock(&display->tme_dfb_display_mutex);
      
      /* do the callout: */
      rc = (conn_mouse != NULL
	    ? ((*conn_mouse->tme_mouse_connection_ctrl)
	       (conn_mouse,
		ctrl))
	    : TME_OK);
	
      /* lock the mutex: */
      tme_mutex_lock(&display->tme_dfb_display_mutex);
      
      /* if the callout was unsuccessful, remember that at some later
	 time this callout should be attempted again: */
      if (rc != TME_OK) {
	later_callouts |= TME_DFB_DISPLAY_CALLOUT_MOUSE_CTRL;
      }
    }
  }
  
  /* put in any later callouts, and clear that callouts are running: */
  display->tme_dfb_display_callout_flags = later_callouts;

  /* yield to DFB: */
  tme_threads_dfb_yield();
}

/* this is a DFB callback for an enter notify event, that has the
   widget grab focus and then continue normal event processing: */
gint
_tme_dfb_display_enter_focus(DfbWidget *widget,
			     GdkEvent *gdk_event_raw,
			     gpointer junk)
{

  /* grab the focus: */
  dfb_widget_grab_focus(widget);

  /* continue normal event processing: */
  return (FALSE);
}

/* this creates a menu of radio buttons: */
DfbWidget *
_tme_dfb_display_menu_radio(void *state,
			    tme_dfb_display_menu_items_t menu_items)
{
  DfbWidget *menu;
  GSList *menu_group;
  struct tme_dfb_display_menu_item menu_item_buffer;
  GCallback menu_func;
  DfbWidget *menu_item;

  /* create the menu: */
  menu = dfb_menu_new();

  /* create the menu items: */
  menu_group = NULL;
  for (menu_item_buffer.tme_dfb_display_menu_item_which = 0;
       ;
       menu_item_buffer.tme_dfb_display_menu_item_which++) {
    menu_func = (*menu_items)(state, &menu_item_buffer);
    if (menu_func == G_CALLBACK(NULL)) {
      break;
    }
    menu_item
      = dfb_radio_menu_item_new_with_label(menu_group,
					   menu_item_buffer.tme_dfb_display_menu_item_string);
    if (menu_item_buffer.tme_dfb_display_menu_item_widget != NULL) {
      *menu_item_buffer.tme_dfb_display_menu_item_widget = menu_item;
    }
    menu_group
      = dfb_radio_menu_item_get_group(DFB_RADIO_MENU_ITEM(menu_item));
    g_signal_connect(menu_item, 
		     "activate",
		     menu_func,
		     (gpointer) state);
    dfb_menu_shell_append(DFB_MENU_SHELL(menu), menu_item);
    dfb_widget_show(menu_item);
  }

  /* return the menu: */
  return (menu);
}

/* this makes a new connection side for a DFB display: */
static int
_tme_dfb_display_connections_new(struct tme_element *element, 
				 const char * const *args, 
				 struct tme_connection **_conns,
				 char **_output)
{
  struct tme_dfb_display *display;

  /* recover our data structure: */
  display = (struct tme_dfb_display *) element->tme_element_private;

  /* we never take any arguments: */
  if (args[1] != NULL) {
    tme_output_append_error(_output,
			    "%s %s, ",
			    args[1],
			    _("unexpected"));
    return (EINVAL);
  }

  /* make any new keyboard connections: */
  _tme_dfb_keyboard_connections_new(display, _conns);

  /* make any new mouse connections: */
  _tme_dfb_mouse_connections_new(display, _conns);

  /* make any new screen connections: */
  _tme_dfb_screen_connections_new(display, _conns);

  /* done: */
  return (TME_OK);
}

/* the new DFB display function: */
TME_ELEMENT_SUB_NEW_DECL(tme_host_dfb,display) {
  struct tme_dfb_display *display;
  int arg_i;
  int usage;
  
  /* check our arguments: */
  usage = FALSE;
  arg_i = 1;
  for (;;) {

    if (0) {
    }

    /* if we've run out of arguments: */
    else if (args[arg_i + 0] == NULL) {

      break;
    }

    /* otherwise this is a bad argument: */
    else {
      tme_output_append_error(_output,
			      "%s %s", 
			      args[arg_i],
			      _("unexpected"));
      usage = TRUE;
      break;
    }
  }

  if (usage) {
    tme_output_append_error(_output,
			    "%s %s",
			    _("usage:"),
			    args[0]);
    return (EINVAL);
  }

  /* call dfb_init if we haven't already: */
  tme_threads_dfb_init();

  /* start our data structure: */
  display = tme_new0(struct tme_dfb_display, 1);
  display->tme_dfb_display_element = element;

  /* create the keyboard: */
  _tme_dfb_keyboard_new(display);

  /* create the mouse: */
  _tme_dfb_mouse_new(display);

  /* create the first screen: */
  _tme_dfb_screen_new(display);

  /* start the threads: */
  tme_mutex_init(&display->tme_dfb_display_mutex);
  tme_thread_create((tme_thread_t) _tme_dfb_screen_th_update, display);

  /* fill the element: */
  element->tme_element_private = display;
  element->tme_element_connections_new = _tme_dfb_display_connections_new;

  return (TME_OK);
}
