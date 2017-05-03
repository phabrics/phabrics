/* $Id: gtk-display.c,v 1.4 2010/06/05 14:28:17 fredette Exp $ */

/* host/disp/display.c - generic display support: */

/*
 * Copyright (c) 2017 Ruben Agin
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
_TME_RCSID("$Id: gtk-display.c,v 1.4 2010/06/05 14:28:17 fredette Exp $");

/* includes: */
#include "display.h"

/* the generic display callout function.  it must be called with the mutex locked: */
void
_tme_display_callout(struct tme_display *display,
			 int new_callouts)
{
  struct tme_keyboard_connection *conn_keyboard;
  struct tme_mouse_connection *conn_mouse;
  int callouts, later_callouts;
  unsigned int ctrl;
  int rc;
  
  /* add in any new callouts: */
  display->tme_display_callout_flags |= new_callouts;

  /* if this function is already running in another thread, simply
     return now.  the other thread will do our work: */
  if (display->tme_display_callout_flags
      & TME_DISPLAY_CALLOUT_RUNNING) {
    return;
  }

  /* callouts are now running: */
  display->tme_display_callout_flags
    |= TME_DISPLAY_CALLOUT_RUNNING;

  /* assume that we won't need any later callouts: */
  later_callouts = 0;

  /* loop while callouts are needed: */
  for (; ((callouts
	   = display->tme_display_callout_flags)
	  & TME_DISPLAY_CALLOUTS_MASK); ) {

    /* clear the needed callouts: */
    display->tme_display_callout_flags
      = (callouts
	 & ~TME_DISPLAY_CALLOUTS_MASK);
    callouts
      &= TME_DISPLAY_CALLOUTS_MASK;

    /* get our keyboard connection: */
    conn_keyboard = display->tme_display_keyboard_connection;

    /* if we need to call out new keyboard control information: */
    if (callouts & TME_DISPLAY_CALLOUT_KEYBOARD_CTRL) {

      /* form the new ctrl: */
      ctrl = 0;
      if (!tme_keyboard_buffer_is_empty(display->tme_display_keyboard_buffer)) {
	ctrl |= TME_KEYBOARD_CTRL_OK_READ;
      }

      /* unlock the mutex: */
      tme_mutex_unlock(&display->tme_display_mutex);
      
      /* do the callout: */
      rc = (conn_keyboard != NULL
	    ? ((*conn_keyboard->tme_keyboard_connection_ctrl)
	       (conn_keyboard,
		ctrl))
	    : TME_OK);
	
      /* lock the mutex: */
      tme_mutex_lock(&display->tme_display_mutex);
      
      /* if the callout was unsuccessful, remember that at some later
	 time this callout should be attempted again: */
      if (rc != TME_OK) {
	later_callouts |= TME_DISPLAY_CALLOUT_KEYBOARD_CTRL;
      }
    }

    /* get our mouse connection: */
    conn_mouse = display->tme_display_mouse_connection;

    /* if we need to call out new mouse control information: */
    if (callouts & TME_DISPLAY_CALLOUT_MOUSE_CTRL) {

      /* form the new ctrl: */
      ctrl = 0;
      if (!tme_mouse_buffer_is_empty(display->tme_display_mouse_buffer)) {
	ctrl |= TME_MOUSE_CTRL_OK_READ;
      }

      /* unlock the mutex: */
      tme_mutex_unlock(&display->tme_display_mutex);
      
      /* do the callout: */
      rc = (conn_mouse != NULL
	    ? ((*conn_mouse->tme_mouse_connection_ctrl)
	       (conn_mouse,
		ctrl))
	    : TME_OK);
	
      /* lock the mutex: */
      tme_mutex_lock(&display->tme_display_mutex);
      
      /* if the callout was unsuccessful, remember that at some later
	 time this callout should be attempted again: */
      if (rc != TME_OK) {
	later_callouts |= TME_DISPLAY_CALLOUT_MOUSE_CTRL;
      }
    }
  }
  
  /* put in any later callouts, and clear that callouts are running: */
  display->tme_display_callout_flags = later_callouts;

}

/* this makes a new connection side for a generic display: */
int
_tme_display_connections_new(struct tme_element *element, 
			     const char * const *args, 
			     struct tme_connection **_conns,
			     char **_output)
{
  struct tme_display *display;

  /* recover our data structure: */
  display = (struct tme_display *) element->tme_element_private;

  /* we never take any arguments: */
  if (args[1] != NULL) {
    tme_output_append_error(_output,
			    "%s %s, ",
			    args[1],
			    _("unexpected"));
    return (EINVAL);
  }

  /* make any new keyboard connections: */
  _tme_keyboard_connections_new(display, _conns);

  /* make any new mouse connections: */
  _tme_mouse_connections_new(display, _conns);

  /* done: */
  return (TME_OK);
}

/* the new generic display function: */
int tme_display_init(struct tme_element *element) {
  struct tme_display *display;

  /* start our data structure: */
  display = tme_new0(struct tme_display, 1);
  display->tme_display_element = element;

  /* create the keyboard: */
  _tme_keyboard_new(display);

  /* create the mouse: */
  _tme_mouse_new(display);

  /* start the threads: */
  tme_mutex_init(&display->tme_display_mutex);

  /* fill the element: */
  element->tme_element_private = display;
  element->tme_element_connections_new = _tme_display_connections_new;

  return (TME_OK);
}
