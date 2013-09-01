/* $Id: dfb-mouse.c,v 1.3 2007/03/03 15:33:22 fredette Exp $ */

/* host/dfb/dfb-mouse.c - DFB mouse support: */

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
_TME_RCSID("$Id: dfb-mouse.c,v 1.3 2007/03/03 15:33:22 fredette Exp $");

/* includes: */
#include "dfb-display.h"
#include <gdk/gdkkeysyms.h>

/* this warps the pointer to the middle of the DfbImage: */
static void
_tme_dfb_mouse_warp_pointer(struct tme_dfb_screen *screen)
{
  gdk_device_warp(screen->tme_dfb_screen_pointer,
		  gdk_screen_get_default(),
		  screen->tme_dfb_screen_mouse_warp_x,
		  screen->tme_dfb_screen_mouse_warp_y);
}

/* this is for debugging only: */
#if 0
#include <stdio.h>
void
_tme_dfb_mouse_debug(const struct tme_mouse_event *event)
{
  fprintf(stderr,
	  "buttons = 0x%02x dx=%d dy=%d\n",
	  event->tme_mouse_event_buttons,
	  event->tme_mouse_event_delta_x,
	  event->tme_mouse_event_delta_y);
}
#else
#define _tme_dfb_mouse_debug(e) do { } while (/* CONSTCOND */ 0)
#endif

/* this is a DFB callback for a mouse event in the framebuffer event box: */
static int
_tme_dfb_mouse_mouse_event(DfbWidget *widget,
			   GdkEvent *gdk_event_raw,
			   struct tme_dfb_screen *screen)
{
  struct tme_dfb_display *display;
  struct tme_mouse_event tme_event;
  gint x;
  gint y;
  guint state, button;
  unsigned int buttons;
  int was_empty;
  int new_callouts;
  int rc;

  /* start the tme event: */
  tme_event.tme_mouse_event_delta_units
    = TME_MOUSE_UNITS_UNKNOWN;

  /* recover our data structure: */
  display = screen->tme_dfb_screen_display;

  /* lock the mutex: */
  tme_mutex_lock(&display->tme_dfb_display_mutex);

  /* if this is motion: */
  if (gdk_event_raw->type == GDK_MOTION_NOTIFY) {

    /* this event must have happened in the dfbimage: */
    assert (gdk_event_raw->motion.window
	    == dfb_widget_get_window(screen->tme_dfb_screen_dfbimage));

    /* set the event time: */
    tme_event.tme_mouse_event_time
      = gdk_event_raw->motion.time;

    /* the buttons haven't changed: */
    tme_event.tme_mouse_event_buttons =
      screen->tme_dfb_screen_mouse_buttons_last;

    /* if the pointer position hasn't changed either, return now.
       every time we warp the pointer we will get a motion event, and
       this should ignore those events: */
    x = gdk_event_raw->motion.x;
    y = gdk_event_raw->motion.y;
    if (x == screen->tme_dfb_screen_mouse_warp_x
	&& y == screen->tme_dfb_screen_mouse_warp_y) {
      
      /* unlock the mutex: */
      tme_mutex_unlock(&display->tme_dfb_display_mutex);

      /* stop propagating this event: */
      return (TRUE);
    }

    /* warp the pointer back to center: */
    _tme_dfb_mouse_warp_pointer(screen);
  }

  /* otherwise, if this is a double- or triple-click: */
  else if (gdk_event_raw->type == GDK_2BUTTON_PRESS
	   || gdk_event_raw->type == GDK_3BUTTON_PRESS) {

    /* we ignore double- and triple-click events, since normal button
       press and release events are always generated also: */
      
    /* unlock the mutex: */
    tme_mutex_unlock(&display->tme_dfb_display_mutex);

    /* stop propagating this event: */
    return (TRUE);
  }

  /* otherwise, this must be a button press or a release: */
  else {
    assert (gdk_event_raw->type == GDK_BUTTON_PRESS
	    || gdk_event_raw->type == GDK_BUTTON_RELEASE);

    /* this event must have happened in the dfbimage: */
    assert (gdk_event_raw->button.window
	    == dfb_widget_get_window(screen->tme_dfb_screen_dfbimage));

    /* set the event time: */
    tme_event.tme_mouse_event_time
      = gdk_event_raw->button.time;

    /* get the pointer position: */
    x = gdk_event_raw->button.x;
    y = gdk_event_raw->button.y;

    /* make the buttons mask: */
    buttons = 0;
    button = gdk_event_raw->button.button;
    state = gdk_event_raw->button.state;
#define _TME_DFB_MOUSE_BUTTON(i, gdk, tme)		\
  if ((button == i)					\
      ? (gdk_event_raw->type == GDK_BUTTON_PRESS)	\
      : (state & gdk))					\
      buttons |= tme
    _TME_DFB_MOUSE_BUTTON(1, GDK_BUTTON1_MASK, TME_MOUSE_BUTTON_0);
    _TME_DFB_MOUSE_BUTTON(2, GDK_BUTTON2_MASK, TME_MOUSE_BUTTON_1);
    _TME_DFB_MOUSE_BUTTON(3, GDK_BUTTON3_MASK, TME_MOUSE_BUTTON_2);
    _TME_DFB_MOUSE_BUTTON(4, GDK_BUTTON4_MASK, TME_MOUSE_BUTTON_3);
    _TME_DFB_MOUSE_BUTTON(5, GDK_BUTTON5_MASK, TME_MOUSE_BUTTON_4);
#undef _TME_DFB_MOUSE_BUTTON
    tme_event.tme_mouse_event_buttons = buttons;
    screen->tme_dfb_screen_mouse_buttons_last = buttons;
  }

  /* make the deltas: */
  tme_event.tme_mouse_event_delta_x = 
    (((int) x)
     - ((int) screen->tme_dfb_screen_mouse_warp_x));
  tme_event.tme_mouse_event_delta_y = 
    (((int) y)
     - ((int) screen->tme_dfb_screen_mouse_warp_y));

  /* assume that we won't need any new callouts: */
  new_callouts = 0;
  
  /* remember if the mouse buffer was empty: */
  was_empty
    = tme_mouse_buffer_is_empty(display->tme_dfb_display_mouse_buffer);

  /* add this tme event to the mouse buffer: */
  _tme_dfb_mouse_debug(&tme_event);
  rc = tme_mouse_buffer_copyin(display->tme_dfb_display_mouse_buffer,
			       &tme_event);
  assert (rc == TME_OK);

  /* if the mouse buffer was empty and now it isn't,
     call out the mouse controls: */
  if (was_empty
      && !tme_mouse_buffer_is_empty(display->tme_dfb_display_mouse_buffer)) {
    new_callouts |= TME_DFB_DISPLAY_CALLOUT_MOUSE_CTRL;
  }

  /* run any callouts: */
  _tme_dfb_display_callout(display, new_callouts);

  /* unlock the mutex: */
  tme_mutex_unlock(&display->tme_dfb_display_mutex);

  /* stop propagating this event: */
  return (TRUE);
}

/* this is a DFB callback for an event in the event box containing the
   mouse label: */
static int
_tme_dfb_mouse_ebox_event(DfbWidget *widget,
			  GdkEvent *gdk_event_raw,
			  struct tme_dfb_screen *screen)
{
  struct tme_dfb_display *display;
  int rc;
  gint junk;
  char *status;

  /* if this is an enter notify event, grab the focus and continue
     propagating the event: */
  if (gdk_event_raw->type == GDK_ENTER_NOTIFY) {
    dfb_widget_grab_focus(widget);
    return (FALSE);
  }

  /* if this is not a key press event, continue propagating it now: */
  if (gdk_event_raw->type != GDK_KEY_PRESS) {
    return (FALSE);
  }

  /* recover our data structure: */
  display = screen->tme_dfb_screen_display;

  /* lock the mutex: */
  tme_mutex_lock(&display->tme_dfb_display_mutex);

  /* the mouse must not be on already: */
  assert (screen->tme_dfb_screen_mouse_keyval
	  == GDK_KEY_VoidSymbol);

  /* this keyval must not be GDK_KEY_VoidSymbol: */
  assert (gdk_event_raw->key.keyval
	  != GDK_KEY_VoidSymbol);
  
  /* set the text on the mouse label: */
  dfb_label_set_text(DFB_LABEL(screen->tme_dfb_screen_mouse_label),
		     _("Mouse is on"));
  
  /* push the mouse status onto the statusbar: */
  status = NULL;
  tme_output_append(&status,
		    _("Press the %s key to turn the mouse off"),
		    gdk_keyval_name(gdk_event_raw->key.keyval));
  dfb_statusbar_push(DFB_STATUSBAR(screen->tme_dfb_screen_mouse_statusbar),
		     screen->tme_dfb_screen_mouse_statusbar_cid,
		     status);
  tme_free(status);
  
  /* if the original events mask on the framebuffer event box have
     never been saved, save them now, and add the mouse events: */
  if (screen->tme_dfb_screen_mouse_events_old == 0) {
    screen->tme_dfb_screen_mouse_events_old
      = gdk_window_get_events(dfb_widget_get_window(screen->tme_dfb_screen_event_box));
    dfb_widget_add_events(screen->tme_dfb_screen_event_box,
			  GDK_POINTER_MOTION_MASK
			  | GDK_BUTTON_PRESS_MASK
			  | GDK_BUTTON_RELEASE_MASK);
  }

  /* get the current width and height of the framebuffer dfbimage, and
     halve them to get the warp center: */
  gdk_window_get_geometry(dfb_widget_get_window(screen->tme_dfb_screen_dfbimage),
			  &junk,
			  &junk,
			  &screen->tme_dfb_screen_mouse_warp_x,
			  &screen->tme_dfb_screen_mouse_warp_y);
  screen->tme_dfb_screen_mouse_warp_x >>= 1;
  screen->tme_dfb_screen_mouse_warp_y >>= 1;
  
  /* warp the pointer to center: */
  _tme_dfb_mouse_warp_pointer(screen);
  
  /* grab the pointer: */
  rc
    = gdk_device_grab(screen->tme_dfb_screen_pointer,
		      dfb_widget_get_window(screen->tme_dfb_screen_dfbimage),
		      GDK_OWNERSHIP_NONE,
		      TRUE,
		      GDK_POINTER_MOTION_MASK
		      | GDK_BUTTON_PRESS_MASK
		      | GDK_BUTTON_RELEASE_MASK,
		      display->tme_dfb_display_mouse_cursor,
		      gdk_event_raw->key.time);
  assert (rc == 0);
  
  /* we are now in mouse mode: */
  screen->tme_dfb_screen_mouse_keyval
    = gdk_event_raw->key.keyval;

  /* unlock the mutex: */
  tme_mutex_unlock(&display->tme_dfb_display_mutex);

  /* stop propagating this event: */
  return (TRUE);
}

/* this turns mouse mode off.  it is called with the mutex locked: */
void
_tme_dfb_mouse_mode_off(struct tme_dfb_screen *screen,
			guint32 time)
{
  /* the mouse must be on: */
  assert (screen->tme_dfb_screen_mouse_keyval
	  != GDK_KEY_VoidSymbol);

  /* ungrab the pointer: */
  gdk_device_ungrab(screen->tme_dfb_screen_pointer, time);

  /* restore the old events mask on the event box: */
  gdk_window_set_events(dfb_widget_get_window(screen->tme_dfb_screen_event_box),
			screen->tme_dfb_screen_mouse_events_old);

  /* pop our message off of the statusbar: */
  dfb_statusbar_pop(DFB_STATUSBAR(screen->tme_dfb_screen_mouse_statusbar),
		    screen->tme_dfb_screen_mouse_statusbar_cid);

  /* restore the text on the mouse label: */
  dfb_label_set_text(DFB_LABEL(screen->tme_dfb_screen_mouse_label),
		     _("Mouse is off"));

  /* the mouse is now off: */
  screen->tme_dfb_screen_mouse_keyval = GDK_KEY_VoidSymbol;
}

/* this is called when the mouse controls change: */
static int
_tme_dfb_mouse_ctrl(struct tme_mouse_connection *conn_mouse, 
		    unsigned int ctrl)
{
  struct tme_dfb_display *display;

  /* recover our data structure: */
  display = conn_mouse
    ->tme_mouse_connection.tme_connection_element->tme_element_private;

  /* XXX TBD */
  abort();

  return (TME_OK);
}

/* this is called to read the mouse: */
static int
_tme_dfb_mouse_read(struct tme_mouse_connection *conn_mouse, 
		    struct tme_mouse_event *event,
		    unsigned int count)
{
  struct tme_dfb_display *display;
  int rc;

  /* recover our data structure: */
  display = conn_mouse
    ->tme_mouse_connection.tme_connection_element->tme_element_private;

  /* lock the mutex: */
  tme_mutex_lock(&display->tme_dfb_display_mutex);

  /* copy an event out of the mouse buffer: */
  rc = tme_mouse_buffer_copyout(display->tme_dfb_display_mouse_buffer,
				event,
				count);

  /* unlock the mutex: */
  tme_mutex_unlock(&display->tme_dfb_display_mutex);

  return (rc);
}

/* this breaks a mouse connection: */
static int
_tme_dfb_mouse_connection_break(struct tme_connection *conn,
				unsigned int state)
{
  abort();
}

/* this makes a new mouse connection: */
static int
_tme_dfb_mouse_connection_make(struct tme_connection *conn,
			       unsigned int state)
{
  struct tme_dfb_display *display;

  /* recover our data structure: */
  display = conn->tme_connection_element->tme_element_private;

  /* both sides must be mouse connections: */
  assert(conn->tme_connection_type
	 == TME_CONNECTION_MOUSE);
  assert(conn->tme_connection_other->tme_connection_type
	 == TME_CONNECTION_MOUSE);

  /* we are always set up to answer calls across the connection, so we
     only have to do work when the connection has gone full, namely
     taking the other side of the connection: */
  if (state == TME_CONNECTION_FULL) {

    /* save our connection: */
    tme_mutex_lock(&display->tme_dfb_display_mutex);
    display->tme_dfb_display_mouse_connection
      = (struct tme_mouse_connection *) conn->tme_connection_other;
    tme_mutex_unlock(&display->tme_dfb_display_mutex);
  }

  return (TME_OK);
}

/* this scores a mouse connection: */
static int
_tme_dfb_mouse_connection_score(struct tme_connection *conn,
				unsigned int *_score)
{
  struct tme_mouse_connection *conn_mouse;

  /* both sides must be mouse connections: */
  assert(conn->tme_connection_type == TME_CONNECTION_MOUSE);
  assert(conn->tme_connection_other->tme_connection_type == TME_CONNECTION_MOUSE);

  /* the other side cannot be a real mouse: */
  conn_mouse
    = (struct tme_mouse_connection *) conn->tme_connection_other;
  *_score = (conn_mouse->tme_mouse_connection_read == NULL);
  return (TME_OK);
}

/* this makes a new connection side for a DFB mouse: */
int
_tme_dfb_mouse_connections_new(struct tme_dfb_display *display, 
			       struct tme_connection **_conns)
{
  struct tme_mouse_connection *conn_mouse;
  struct tme_connection *conn;

  /* if we don't have a mouse connection yet: */
  if (display->tme_dfb_display_mouse_connection == NULL) {

    /* create our side of a mouse connection: */
    conn_mouse = tme_new0(struct tme_mouse_connection, 1);
    conn = &conn_mouse->tme_mouse_connection;

    /* fill in the generic connection: */
    conn->tme_connection_next = *_conns;
    conn->tme_connection_type = TME_CONNECTION_MOUSE;
    conn->tme_connection_score = _tme_dfb_mouse_connection_score;
    conn->tme_connection_make = _tme_dfb_mouse_connection_make;
    conn->tme_connection_break = _tme_dfb_mouse_connection_break;

    /* fill in the mouse connection: */
    conn_mouse->tme_mouse_connection_ctrl = _tme_dfb_mouse_ctrl;
    conn_mouse->tme_mouse_connection_read = _tme_dfb_mouse_read;

    /* return the connection side possibility: */
    *_conns = conn;
  }

  /* done: */
  return (TME_OK);
}

/* this attaches the DFB mouse to a new screen: */
void
_tme_dfb_mouse_attach(struct tme_dfb_screen *screen)
{
  struct tme_dfb_display *display;
  DfbWidget *hbox0;
  DfbWidget *ebox;

  /* get the display: */
  display = screen->tme_dfb_screen_display;

  /* create the horizontal packing box for the mouse controls: */
  hbox0 = dfb_box_new(DFB_ORIENTATION_HORIZONTAL, 0);

  /* pack the horizontal packing box into the outer vertical packing box: */
  dfb_box_pack_start(DFB_BOX(screen->tme_dfb_screen_vbox0), 
		     hbox0,
		     FALSE, FALSE, 0);

  /* show the horizontal packing box: */
  dfb_widget_show(hbox0);

  /* create the event box for the mouse on label: */
  ebox = dfb_event_box_new();

  /* pack the event box into the horizontal packing box: */
  dfb_box_pack_start(DFB_BOX(hbox0), 
		     ebox,
		     FALSE, FALSE, 0);

  /* set the tip on the event box, which will eventually contain the mouse on label: */
  dfb_widget_set_tooltip_text(ebox,
			      "Press a key here to turn the mouse on.  The same key " \
			      "will turn the mouse off.");

  /* make sure the event box gets key_press events: */
  dfb_widget_add_events(ebox, 
			GDK_KEY_PRESS_MASK);

  /* set a signal handler for the event box events: */
  g_signal_connect(ebox,
		   "event",
		   G_CALLBACK(_tme_dfb_mouse_ebox_event),
		   screen);

  /* the event box can focus: */
  dfb_widget_set_can_focus(ebox, TRUE);

  /* show the event box: */
  dfb_widget_show(ebox);

  /* create the mouse on label: */
  screen->tme_dfb_screen_mouse_label
    = dfb_label_new(_("Mouse is off"));

  /* add the mouse on label to the event box: */
  dfb_container_add(DFB_CONTAINER(ebox), 
		    screen->tme_dfb_screen_mouse_label);

  /* show the mouse on label: */
  dfb_widget_show(screen->tme_dfb_screen_mouse_label);

  /* create the mouse statusbar: */
  screen->tme_dfb_screen_mouse_statusbar
    = dfb_statusbar_new();

  /* pack the mouse statusbar into the horizontal packing box: */
  dfb_box_pack_start(DFB_BOX(hbox0), 
		     screen->tme_dfb_screen_mouse_statusbar,
		     TRUE, TRUE, 10);

  /* show the mouse statusbar: */
  dfb_widget_show(screen->tme_dfb_screen_mouse_statusbar);

  /* push an initial message onto the statusbar: */
  screen->tme_dfb_screen_mouse_statusbar_cid
    = dfb_statusbar_get_context_id(DFB_STATUSBAR(screen->tme_dfb_screen_mouse_statusbar),
				   "mouse context");
  dfb_statusbar_push(DFB_STATUSBAR(screen->tme_dfb_screen_mouse_statusbar),
		     screen->tme_dfb_screen_mouse_statusbar_cid,
		     _("The Machine Emulator"));

  /* although the event mask doesn't include these events yet,
     set a signal handler for the mouse events: */
  g_signal_connect(screen->tme_dfb_screen_event_box,
		   "motion_notify_event",
		   G_CALLBACK(_tme_dfb_mouse_mouse_event), 
		   screen);
  g_signal_connect(screen->tme_dfb_screen_event_box,
		   "button_press_event",
		   G_CALLBACK(_tme_dfb_mouse_mouse_event), 
		   screen);
  g_signal_connect(screen->tme_dfb_screen_event_box,
		   "button_release_event",
		   G_CALLBACK(_tme_dfb_mouse_mouse_event), 
		   screen);

  /* mouse mode is off: */
  screen->tme_dfb_screen_mouse_keyval = GDK_KEY_VoidSymbol;
}

/* this initializes mouse part of the display: */
void
_tme_dfb_mouse_new(struct tme_dfb_display *display)
{
  /* we have no mouse connection: */
  display->tme_dfb_display_mouse_connection = NULL;
  
  /* allocate the mouse buffer: */
  display->tme_dfb_display_mouse_buffer
    = tme_mouse_buffer_new(1024);

  display->tme_dfb_display_mouse_cursor
    = gdk_cursor_new(GDK_BLANK_CURSOR);
}
