/* $Id: gtk-mouse.c,v 1.3 2007/03/03 15:33:22 fredette Exp $ */

/* host/gtk/gtk-mouse.c - GTK mouse support: */

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
_TME_RCSID("$Id: gtk-mouse.c,v 1.3 2007/03/03 15:33:22 fredette Exp $");

/* includes: */
#include "gtk-display.h"
#include <gdk/gdkkeysyms.h>

/* this warps the pointer to the middle of the Gtkframe: */
static void
_tme_gtk_mouse_warp_pointer(struct tme_gtk_screen *screen)
{
  gdk_device_warp(screen->tme_gtk_screen_pointer,
		  gdk_screen_get_default(),
		  screen->tme_gtk_screen_mouse_warp_x,
		  screen->tme_gtk_screen_mouse_warp_y);
}

/* this is a GTK callback for a mouse event in the framebuffer event box: */
static int
_tme_gtk_mouse_mouse_event(GtkWidget *widget,
			   GdkEvent *gdk_event_raw,
			   struct tme_gtk_screen *screen)
{
  struct tme_display *display;
  struct tme_mouse_event tme_event;
  gint x;
  gint y;
  guint state, button;
  unsigned int buttons;
  int was_empty;
  int new_callouts;
  int rc;

  _tme_thread_resumed();

  /* start the tme event: */
  tme_event.tme_mouse_event_delta_units
    = TME_MOUSE_UNITS_UNKNOWN;

  /* recover our data structure: */
  display = screen->screen.tme_screen_display;

  /* lock the mutex: */
  tme_mutex_lock(&display->tme_display_mutex);

  /* if this is motion: */
  if (gdk_event_raw->type == GDK_MOTION_NOTIFY) {

    /* this event must have happened in the gtkframe: */
    assert (gdk_event_raw->motion.window
	    == gtk_widget_get_window(screen->tme_gtk_screen_gtkframe));

    /* set the event time: */
    tme_event.tme_mouse_event_time
      = gdk_event_raw->motion.time;

    /* the buttons haven't changed: */
    tme_event.tme_mouse_event_buttons =
      screen->tme_gtk_screen_mouse_buttons_last;

    /* if the pointer position hasn't changed either, return now.
       every time we warp the pointer we will get a motion event, and
       this should ignore those events: */
    x = gdk_event_raw->motion.x_root;
    y = gdk_event_raw->motion.y_root;
    if (x == screen->tme_gtk_screen_mouse_warp_x
	&& y == screen->tme_gtk_screen_mouse_warp_y) {
      
      /* unlock the mutex: */
      tme_mutex_unlock(&display->tme_display_mutex);

      _tme_thread_suspended();

      /* stop propagating this event: */
      return (TRUE);
    }

    /* warp the pointer back to center: */
    _tme_gtk_mouse_warp_pointer(screen);
  }

  /* otherwise, if this is a double- or triple-click: */
  else if (gdk_event_raw->type == GDK_2BUTTON_PRESS
	   || gdk_event_raw->type == GDK_3BUTTON_PRESS) {

    /* we ignore double- and triple-click events, since normal button
       press and release events are always generated also: */
      
    /* unlock the mutex: */
    tme_mutex_unlock(&display->tme_display_mutex);

    _tme_thread_suspended();

    /* stop propagating this event: */
    return (TRUE);
  }

  /* otherwise, this must be a button press or a release: */
  else {
    assert (gdk_event_raw->type == GDK_BUTTON_PRESS
	    || gdk_event_raw->type == GDK_BUTTON_RELEASE);

    /* this event must have happened in the gtkframe: */
    assert (gdk_event_raw->button.window
	    == gtk_widget_get_window(screen->tme_gtk_screen_gtkframe));

    /* set the event time: */
    tme_event.tme_mouse_event_time
      = gdk_event_raw->button.time;

    /* get the pointer position: */
    x = gdk_event_raw->button.x_root;
    y = gdk_event_raw->button.y_root;

    /* make the buttons mask: */
    buttons = 0;
    button = gdk_event_raw->button.button;
    state = gdk_event_raw->button.state;
#define _TME_GTK_MOUSE_BUTTON(i, gdk, tme)		\
  if ((button == i)					\
      ? (gdk_event_raw->type == GDK_BUTTON_PRESS)	\
      : (state & gdk))					\
      buttons |= tme
    _TME_GTK_MOUSE_BUTTON(1, GDK_BUTTON1_MASK, TME_MOUSE_BUTTON_0);
    _TME_GTK_MOUSE_BUTTON(2, GDK_BUTTON2_MASK, TME_MOUSE_BUTTON_1);
    _TME_GTK_MOUSE_BUTTON(3, GDK_BUTTON3_MASK, TME_MOUSE_BUTTON_2);
    _TME_GTK_MOUSE_BUTTON(4, GDK_BUTTON4_MASK, TME_MOUSE_BUTTON_3);
    _TME_GTK_MOUSE_BUTTON(5, GDK_BUTTON5_MASK, TME_MOUSE_BUTTON_4);
#undef _TME_GTK_MOUSE_BUTTON
    tme_event.tme_mouse_event_buttons = buttons;
    screen->tme_gtk_screen_mouse_buttons_last = buttons;
  }

  /* make the deltas: */
  tme_event.tme_mouse_event_delta_x = 
    (((int) x)
     - ((int) screen->tme_gtk_screen_mouse_warp_x));
  tme_event.tme_mouse_event_delta_y = 
    (((int) y)
     - ((int) screen->tme_gtk_screen_mouse_warp_y));

  _tme_mouse_mouse_event(&tme_event, display);
  
  /* unlock the mutex: */
  tme_mutex_unlock(&display->tme_display_mutex);

  _tme_thread_suspended();

  /* stop propagating this event: */
  return (TRUE);
}

/* this is a GTK callback for an event in the event box containing the
   mouse label: */
static int
_tme_gtk_mouse_ebox_event(GtkWidget *widget,
			  GdkEvent *gdk_event_raw,
			  struct tme_gtk_screen *screen)
{
  struct tme_display *display;
  int rc;
  char *status;
  GdkWindow *window;

  /* if this is an enter notify event, grab the focus and continue
     propagating the event: */
  if (gdk_event_raw->type == GDK_ENTER_NOTIFY) {
    gtk_widget_grab_focus(widget);
    return (FALSE);
  }

  /* if this is not a key press event, continue propagating it now: */
  if (gdk_event_raw->type != GDK_KEY_PRESS) {
    return (FALSE);
  }

  _tme_thread_resumed();

  /* recover our data structure: */
  display = screen->screen.tme_screen_display;

  /* lock the mutex: */
  tme_mutex_lock(&display->tme_display_mutex);

  /* the mouse must not be on already: */
  assert (screen->tme_gtk_screen_mouse_keyval
	  == GDK_KEY_VoidSymbol);

  /* this keyval must not be GDK_KEY_VoidSymbol: */
  assert (gdk_event_raw->key.keyval
	  != GDK_KEY_VoidSymbol);
  
  /* set the text on the mouse label: */
  gtk_label_set_text(GTK_LABEL(screen->tme_gtk_screen_mouse_label),
		     _("Mouse is on"));
  
  /* push the mouse status onto the statusbar: */
  status = NULL;
  tme_output_append(&status,
		    _("Press the %s key to turn the mouse off"),
		    gdk_keyval_name(gdk_event_raw->key.keyval));
  gtk_statusbar_push(GTK_STATUSBAR(screen->tme_gtk_screen_mouse_statusbar),
		     screen->tme_gtk_screen_mouse_statusbar_cid,
		     status);
  tme_free(status);
  
  window = gtk_widget_get_window(screen->tme_gtk_screen_gtkframe);

  /* if the original events mask on the framebuffer event box have
     never been saved, save them now, and add the mouse events: */
  if (screen->tme_gtk_screen_mouse_events_old == 0) {
    screen->tme_gtk_screen_mouse_events_old
      = gdk_window_get_events(window);
    gtk_widget_add_events(screen->tme_gtk_screen_gtkframe,
			  GDK_POINTER_MOTION_MASK
			  | GDK_BUTTON_PRESS_MASK
			  | GDK_BUTTON_RELEASE_MASK);
  }

  /* get the current width and height of the framebuffer gtkframe, and
     halve them to get the warp center: */
  gdk_window_get_root_coords(window,
			     gdk_window_get_width(window)>>1,
			     gdk_window_get_height(window)>>1,
			     &screen->tme_gtk_screen_mouse_warp_x,
			     &screen->tme_gtk_screen_mouse_warp_y);

  /* warp the pointer to center: */
  _tme_gtk_mouse_warp_pointer(screen);
  
  /* grab the pointer: */
  rc
    = gdk_device_grab(screen->tme_gtk_screen_pointer,
		      window,
		      GDK_OWNERSHIP_NONE,
		      TRUE,
		      GDK_POINTER_MOTION_MASK
		      | GDK_BUTTON_PRESS_MASK
		      | GDK_BUTTON_RELEASE_MASK,
		      display->tme_display_mouse_cursor,
		      gdk_event_raw->key.time);
  assert (rc == 0);
  
  /* we are now in mouse mode: */
  screen->tme_gtk_screen_mouse_keyval
    = gdk_event_raw->key.keyval;

  /* unlock the mutex: */
  tme_mutex_unlock(&display->tme_display_mutex);

  _tme_thread_suspended();

  /* stop propagating this event: */
  return (TRUE);
}

/* this turns mouse mode off.  it is called with the mutex locked: */
void
_tme_gtk_mouse_mode_off(struct tme_gtk_screen *screen,
			guint32 time)
{
  /* the mouse must be on: */
  assert (screen->tme_gtk_screen_mouse_keyval
	  != GDK_KEY_VoidSymbol);

  /* ungrab the pointer: */
  gdk_device_ungrab(screen->tme_gtk_screen_pointer, time);

  /* restore the old events mask on the event box: */
  gdk_window_set_events(gtk_widget_get_window(screen->tme_gtk_screen_gtkframe),
			screen->tme_gtk_screen_mouse_events_old);

  /* pop our message off of the statusbar: */
  gtk_statusbar_pop(GTK_STATUSBAR(screen->tme_gtk_screen_mouse_statusbar),
		    screen->tme_gtk_screen_mouse_statusbar_cid);

  /* restore the text on the mouse label: */
  gtk_label_set_text(GTK_LABEL(screen->tme_gtk_screen_mouse_label),
		     _("Mouse is off"));

  /* the mouse is now off: */
  screen->tme_gtk_screen_mouse_keyval = GDK_KEY_VoidSymbol;
}

/* this attaches the GTK mouse to a new screen: */
void
_tme_gtk_mouse_attach(struct tme_gtk_screen *screen)
{
  struct tme_display *display;
  GtkWidget *hbox0;
  GtkWidget *ebox;

  /* get the display: */
  display = screen->screen.tme_screen_display;

  /* create the horizontal packing box for the mouse controls: */
  hbox0 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  /* pack the horizontal packing box into the outer vertical packing box: */
  gtk_box_pack_start(GTK_BOX(screen->tme_gtk_screen_vbox0), 
		     hbox0,
		     FALSE, FALSE, 0);

  /* show the horizontal packing box: */
  gtk_widget_show(hbox0);

  /* create the event box for the mouse on label: */
  ebox = gtk_event_box_new();

  /* pack the event box into the horizontal packing box: */
  gtk_box_pack_start(GTK_BOX(hbox0), 
		     ebox,
		     FALSE, FALSE, 0);

  /* set the tip on the event box, which will eventually contain the mouse on label: */
  gtk_widget_set_tooltip_text(ebox,
			      "Press a key here to turn the mouse on.  The same key " \
			      "will turn the mouse off.");

  /* make sure the event box gets key_press events: */
  gtk_widget_add_events(ebox, 
			GDK_KEY_PRESS_MASK);

  /* set a signal handler for the event box events: */
  g_signal_connect(ebox,
		   "event",
		   G_CALLBACK(_tme_gtk_mouse_ebox_event),
		   screen);

  /* the event box can focus: */
  gtk_widget_set_can_focus(ebox, TRUE);

  /* show the event box: */
  gtk_widget_show(ebox);

  /* create the mouse on label: */
  screen->tme_gtk_screen_mouse_label
    = gtk_label_new(_("Mouse is off"));

  /* add the mouse on label to the event box: */
  gtk_container_add(GTK_CONTAINER(ebox), 
		    screen->tme_gtk_screen_mouse_label);

  /* show the mouse on label: */
  gtk_widget_show(screen->tme_gtk_screen_mouse_label);

  /* create the mouse statusbar: */
  screen->tme_gtk_screen_mouse_statusbar
    = gtk_statusbar_new();

  /* pack the mouse statusbar into the horizontal packing box: */
  gtk_box_pack_start(GTK_BOX(hbox0), 
		     screen->tme_gtk_screen_mouse_statusbar,
		     TRUE, TRUE, 10);

  /* show the mouse statusbar: */
  gtk_widget_show(screen->tme_gtk_screen_mouse_statusbar);

  /* push an initial message onto the statusbar: */
  screen->tme_gtk_screen_mouse_statusbar_cid
    = gtk_statusbar_get_context_id(GTK_STATUSBAR(screen->tme_gtk_screen_mouse_statusbar),
				   "mouse context");
  gtk_statusbar_push(GTK_STATUSBAR(screen->tme_gtk_screen_mouse_statusbar),
		     screen->tme_gtk_screen_mouse_statusbar_cid,
		     _("The Machine Emulator"));

  /* although the event mask doesn't include these events yet,
     set a signal handler for the mouse events: */
  g_signal_connect(screen->tme_gtk_screen_gtkframe,
		   "motion_notify_event",
		   G_CALLBACK(_tme_gtk_mouse_mouse_event), 
		   screen);
  g_signal_connect(screen->tme_gtk_screen_gtkframe,
		   "button_press_event",
		   G_CALLBACK(_tme_gtk_mouse_mouse_event), 
		   screen);
  g_signal_connect(screen->tme_gtk_screen_gtkframe,
		   "button_release_event",
		   G_CALLBACK(_tme_gtk_mouse_mouse_event), 
		   screen);

  /* mouse mode is off: */
  screen->tme_gtk_screen_mouse_keyval = GDK_KEY_VoidSymbol;
}
