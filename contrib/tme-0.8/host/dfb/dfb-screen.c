/* $Id: dfb-screen.c,v 1.11 2009/08/30 21:39:03 fredette Exp $ */

/* host/dfb/dfb-screen.c - DFB screen support: */

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
_TME_RCSID("$Id: dfb-screen.c,v 1.11 2009/08/30 21:39:03 fredette Exp $");

/* we are aware of the problems with gdk_image_new_bitmap, and we cope
   with them, so we define GDK_ENABLE_BROKEN to get its prototype
   under DFB 2: */
#define GDK_ENABLE_BROKEN

/* includes: */
#include "dfb-display.h"
#include <stdlib.h>

/* macros: */

/* the DFB 1.x dfb_image_new function is the DFB 2.x
   dfb_image_new_from_image function: */
#if DFB_MAJOR_VERSION == 1
#define dfb_image_new_from_image dfb_image_new
#endif /* DFB_MAJOR_VERSION == 1 */

/* the DFB screens update thread: */
void
_tme_dfb_screen_th_update(struct tme_dfb_display *display)
{
  struct tme_dfb_screen *screen;
  struct tme_fb_connection *conn_fb_other;
  int changed;
  int rc;
  
  /* loop forever: */
  for (;;) {

    /* lock the mutex: */
    tme_mutex_lock(&display->tme_dfb_display_mutex);

    /* loop over all screens: */
    for (screen = display->tme_dfb_display_screens;
	 screen != NULL;
	 screen = screen->tme_dfb_screen_next) {

      /* skip this screen if it's unconnected: */
      if (screen->tme_dfb_screen_fb == NULL) {
	continue;
      }

      /* get the other side of this connection: */
      conn_fb_other
	= ((struct tme_fb_connection *) 
	   screen->tme_dfb_screen_fb->tme_fb_connection.tme_connection_other);

      /* if the framebuffer has an update function, call it: */
      if (conn_fb_other->tme_fb_connection_update != NULL) {
	rc = (*conn_fb_other->tme_fb_connection_update)(conn_fb_other);
	assert (rc == TME_OK);
      }

      /* if this framebuffer needs a full redraw: */
      if (screen->tme_dfb_screen_full_redraw) {

	/* force the next translation to retranslate the entire buffer: */
	tme_fb_xlat_redraw(conn_fb_other);
	conn_fb_other->tme_fb_connection_offset_updated_first = 0;
	conn_fb_other->tme_fb_connection_offset_updated_last = 0 - (tme_uint32_t) 1;

	/* clear the full redraw flag: */
	screen->tme_dfb_screen_full_redraw = FALSE;
      }

      /* translate this framebuffer's contents: */
      changed = (*screen->tme_dfb_screen_fb_xlat)
	(((struct tme_fb_connection *) 
	  screen->tme_dfb_screen_fb->tme_fb_connection.tme_connection_other),
	 screen->tme_dfb_screen_fb);

      /* if those contents changed, redraw the widget: */
      if (changed) {
	dfb_widget_queue_draw(screen->tme_dfb_screen_dfbimage);
      }
    }

    /* unlock the mutex: */
    tme_mutex_unlock(&display->tme_dfb_display_mutex);

    /* update again in .5 seconds: */
    tme_thread_sleep_yield(0, 500000);
  }
  /* NOTREACHED */
}

/* this recovers the bits-per-pixel value for a GdkPixbuf: */
static unsigned int
_tme_dfb_gdkpixbuf_bipp(GdkPixbuf *image, unsigned int *depth)
{
  unsigned int bipc = gdk_pixbuf_get_bits_per_sample(image),
    bipp = bipc * gdk_pixbuf_get_n_channels(image);

  if(depth) *depth = bipp - (gdk_pixbuf_get_has_alpha(image) ? bipc : 0);

  return bipp;
}

/* this recovers the scanline-pad value for a GdkPixbuf: */
static unsigned int
_tme_dfb_gdkpixbuf_scanline_pad(GdkPixbuf *image)
{
  int bpl = gdk_pixbuf_get_rowstride(image);
  
  if ((bpl % sizeof(tme_uint32_t)) == 0) {
    return (32);
  }
  if ((bpl % sizeof(tme_uint16_t)) == 0) {
    return (16);
  }
  return (8);
}

/* this is called for a mode change: */
int
_tme_dfb_screen_mode_change(struct tme_fb_connection *conn_fb)
{
  struct tme_dfb_display *display;
  struct tme_dfb_screen *screen;
  struct tme_fb_connection *conn_fb_other;
  struct tme_fb_xlat fb_xlat_q;
  const struct tme_fb_xlat *fb_xlat_a;
  int scale;
  unsigned long fb_area, avail_area, percentage;
  gint width, height;
  gint height_extra;
  const void *map_g_old;
  const void *map_r_old;
  const void *map_b_old;
  const tme_uint32_t *map_pixel_old;
  tme_uint32_t map_pixel_count_old;  
  tme_uint32_t colorset;
  GdkPixbuf *gdkpixbuf;
  tme_uint32_t color_count;
  struct tme_fb_color *colors_tme;
#if 0
  cairo_t *cr;
  cairo_surface_t *surface;
  cairo_surface_type_t stype;
  int stride;
#endif

  /* recover our data structures: */
  display = conn_fb->tme_fb_connection.tme_connection_element->tme_element_private;
  conn_fb_other = (struct tme_fb_connection *) conn_fb->tme_fb_connection.tme_connection_other;

  /* lock our mutex: */
  tme_mutex_lock(&display->tme_dfb_display_mutex);

  /* find the screen that this framebuffer connection references: */
  for (screen = display->tme_dfb_display_screens;
       (screen != NULL
	&& screen->tme_dfb_screen_fb != conn_fb);
       screen = screen->tme_dfb_screen_next);
  assert (screen != NULL);

  /* if the user hasn't specified a scaling, pick one: */
  scale = screen->tme_dfb_screen_fb_scale;
  if (scale < 0) {

    /* calulate the areas, in square pixels, of the emulated
       framebuffer and the host's screen: */
    fb_area = (conn_fb_other->tme_fb_connection_width
	       * conn_fb_other->tme_fb_connection_height);
    avail_area = (gdk_screen_width()
		  * gdk_screen_height());

    /* see what percentage of the host's screen would be taken up by
       an unscaled emulated framebuffer: */
    percentage = (fb_area * 100) / avail_area;

    /* if this is at least 70%, halve the emulated framebuffer, else
       if this is 30% or less, double the emulated framebuffer: */
    if (percentage >= 70) {
      scale = TME_FB_XLAT_SCALE_HALF;
    }
    else if (percentage <= 30) {
      scale = TME_FB_XLAT_SCALE_DOUBLE;
    }
    else {
      scale = TME_FB_XLAT_SCALE_NONE;
    }

    screen->tme_dfb_screen_fb_scale = -scale;
  }

  /* get the required dimensions for the GdkPixbuf: */
  width = ((conn_fb_other->tme_fb_connection_width
	    * scale)
	   / TME_FB_XLAT_SCALE_NONE);
  height = ((conn_fb_other->tme_fb_connection_height
	     * scale)
	    / TME_FB_XLAT_SCALE_NONE);
  /* NB: we need to allocate an extra scanline's worth (or, if we're
     doubling, an extra two scanlines' worth) of image, because the
     framebuffer translation functions can sometimes overtranslate
     (see the explanation of TME_FB_XLAT_RUN in fb-xlat-auto.sh): */
  height_extra
    = (scale == TME_FB_XLAT_SCALE_DOUBLE
       ? 2
       : 1);

  /* if the previous gdkpixbuf isn't the right size: */
  gdkpixbuf = screen->tme_dfb_screen_gdkpixbuf;
  if (gdk_pixbuf_get_width(gdkpixbuf) != width
      || gdk_pixbuf_get_height(gdkpixbuf) != (height + height_extra)) {
    /* allocate a new gdkpixbuf: */
    gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height + height_extra);
    /* set the new image on the image widget: */
    dfb_image_set_from_pixbuf(DFB_IMAGE(screen->tme_dfb_screen_dfbimage),
			      gdkpixbuf);
    /* destroy the previous gdkpixbuf and remember the new one: */
    g_object_unref(screen->tme_dfb_screen_gdkpixbuf);
    screen->tme_dfb_screen_gdkpixbuf = gdkpixbuf;
#if 0
    cr = gdk_cairo_create(dfb_widget_get_window(screen->tme_dfb_screen_dfbimage));
    surface = cairo_get_target(cr);
    stype = cairo_surface_get_type(surface);
#endif
  }

  /* remember all previously allocated maps and colors, but otherwise
     remove them from our framebuffer structure: */
  map_g_old = conn_fb->tme_fb_connection_map_g;
  map_r_old = conn_fb->tme_fb_connection_map_r;
  map_b_old = conn_fb->tme_fb_connection_map_b;
  map_pixel_old = conn_fb->tme_fb_connection_map_pixel;
  map_pixel_count_old = conn_fb->tme_fb_connection_map_pixel_count;
  conn_fb->tme_fb_connection_map_g = NULL;
  conn_fb->tme_fb_connection_map_r = NULL;
  conn_fb->tme_fb_connection_map_b = NULL;
  conn_fb->tme_fb_connection_map_pixel = NULL;
  conn_fb->tme_fb_connection_map_pixel_count = 0;

  /* update our framebuffer connection: */
  conn_fb->tme_fb_connection_width = gdk_pixbuf_get_width(gdkpixbuf);
  conn_fb->tme_fb_connection_height = gdk_pixbuf_get_height(gdkpixbuf);
  conn_fb->tme_fb_connection_bits_per_pixel = _tme_dfb_gdkpixbuf_bipp(gdkpixbuf, &conn_fb->tme_fb_connection_depth);
  conn_fb->tme_fb_connection_skipx = gdk_pixbuf_get_rowstride(gdkpixbuf) / gdk_pixbuf_get_n_channels(gdkpixbuf) - conn_fb->tme_fb_connection_width;
  conn_fb->tme_fb_connection_scanline_pad = _tme_dfb_gdkpixbuf_scanline_pad(gdkpixbuf);
  conn_fb->tme_fb_connection_order = TME_ENDIAN_NATIVE;
  conn_fb->tme_fb_connection_buffer = gdk_pixbuf_get_pixels(gdkpixbuf);
  conn_fb->tme_fb_connection_class = TME_FB_XLAT_CLASS_COLOR;
  conn_fb->tme_fb_connection_mask_g = 0x00ff00;
  conn_fb->tme_fb_connection_mask_r = 0x0000ff;
  conn_fb->tme_fb_connection_mask_b = 0xff0000;
  
  /* get the needed colors: */
  colorset = tme_fb_xlat_colors_get(conn_fb_other, scale, conn_fb, &colors_tme);
  color_count = conn_fb->tme_fb_connection_map_pixel_count;

  /* if we need to allocate colors, but the colorset is not tied to
     the source framebuffer characteristics, and is identical to the
     currently allocated colorset, we can reuse the previously
     allocated maps and colors: */
  if (color_count > 0
      && colorset != TME_FB_COLORSET_NONE
      && colorset == screen->tme_dfb_screen_colorset) {

    /* free the requested color array: */
    tme_free(colors_tme);

    /* restore the previously allocated maps and colors: */
    conn_fb->tme_fb_connection_map_g = map_g_old;
    conn_fb->tme_fb_connection_map_r = map_r_old;
    conn_fb->tme_fb_connection_map_b = map_b_old;
    conn_fb->tme_fb_connection_map_pixel = map_pixel_old;
    conn_fb->tme_fb_connection_map_pixel_count = map_pixel_count_old;
  }


  /* otherwise, we may need to free and/or allocate colors: */
  else {

    /* save the colorset signature: */
    screen->tme_dfb_screen_colorset = colorset;

    /* free any previously allocated maps and colors: */
    if (map_g_old != NULL) {
      tme_free((void *) map_g_old);
    }
    if (map_r_old != NULL) {
      tme_free((void *) map_r_old);
    }
    if (map_b_old != NULL) {
      tme_free((void *) map_b_old);
    }
    if (map_pixel_old != NULL) {
      tme_free((void *) map_pixel_old);
    }

    /* if we need to allocate colors: */
    if (color_count > 0) {
      /* set the needed colors: */
      tme_fb_xlat_colors_set(conn_fb_other, scale, conn_fb, colors_tme);
    }
  }

  /* compose the framebuffer translation question: */
  fb_xlat_q.tme_fb_xlat_width			= conn_fb_other->tme_fb_connection_width;
  fb_xlat_q.tme_fb_xlat_height			= conn_fb_other->tme_fb_connection_height;
  fb_xlat_q.tme_fb_xlat_scale			= (unsigned int) scale;
  fb_xlat_q.tme_fb_xlat_src_depth		= conn_fb_other->tme_fb_connection_depth;
  fb_xlat_q.tme_fb_xlat_src_bits_per_pixel	= conn_fb_other->tme_fb_connection_bits_per_pixel;
  fb_xlat_q.tme_fb_xlat_src_skipx		= conn_fb_other->tme_fb_connection_skipx;
  fb_xlat_q.tme_fb_xlat_src_scanline_pad	= conn_fb_other->tme_fb_connection_scanline_pad;
  fb_xlat_q.tme_fb_xlat_src_order		= conn_fb_other->tme_fb_connection_order;
  fb_xlat_q.tme_fb_xlat_src_class		= conn_fb_other->tme_fb_connection_class;
  fb_xlat_q.tme_fb_xlat_src_map			= (conn_fb_other->tme_fb_connection_map_g != NULL
						   ? TME_FB_XLAT_MAP_INDEX
						   : TME_FB_XLAT_MAP_LINEAR);
  fb_xlat_q.tme_fb_xlat_src_map_bits		= conn_fb_other->tme_fb_connection_map_bits;
  fb_xlat_q.tme_fb_xlat_src_mask_g		= conn_fb_other->tme_fb_connection_mask_g;
  fb_xlat_q.tme_fb_xlat_src_mask_r		= conn_fb_other->tme_fb_connection_mask_r;
  fb_xlat_q.tme_fb_xlat_src_mask_b		= conn_fb_other->tme_fb_connection_mask_b;
  fb_xlat_q.tme_fb_xlat_dst_depth		= conn_fb->tme_fb_connection_depth;
  fb_xlat_q.tme_fb_xlat_dst_bits_per_pixel	= conn_fb->tme_fb_connection_bits_per_pixel;
  fb_xlat_q.tme_fb_xlat_dst_skipx		= conn_fb->tme_fb_connection_skipx;
  fb_xlat_q.tme_fb_xlat_dst_scanline_pad	= conn_fb->tme_fb_connection_scanline_pad;
  fb_xlat_q.tme_fb_xlat_dst_order		= conn_fb->tme_fb_connection_order;
  fb_xlat_q.tme_fb_xlat_dst_map			= (conn_fb->tme_fb_connection_map_g != NULL
						   ? TME_FB_XLAT_MAP_INDEX
						   : TME_FB_XLAT_MAP_LINEAR);
  fb_xlat_q.tme_fb_xlat_dst_mask_g		= conn_fb->tme_fb_connection_mask_g;
  fb_xlat_q.tme_fb_xlat_dst_mask_r		= conn_fb->tme_fb_connection_mask_r;
  fb_xlat_q.tme_fb_xlat_dst_mask_b		= conn_fb->tme_fb_connection_mask_b;

  /* ask the framebuffer translation question: */
  fb_xlat_a = tme_fb_xlat_best(&fb_xlat_q);

  /* if this translation isn't optimal, log a note: */
  if (!tme_fb_xlat_is_optimal(fb_xlat_a)) {
    tme_log(&display->tme_dfb_display_element->tme_element_log_handle, 0, TME_OK,
	    (&display->tme_dfb_display_element->tme_element_log_handle,
	     _("no optimal framebuffer translation function available")));
  }

  /* save the translation function: */
  screen->tme_dfb_screen_fb_xlat = fb_xlat_a->tme_fb_xlat_func;

  /* force the next translation to do a complete redraw: */
  screen->tme_dfb_screen_full_redraw = TRUE;

  /* unlock our mutex: */
  tme_mutex_unlock(&display->tme_dfb_display_mutex);

  /* done: */
  return (TME_OK);
}

/* this sets the screen size: */
static void
_tme_dfb_screen_scale_set(DfbWidget *widget,
			  struct tme_dfb_screen *screen,
			  int scale_new)
{
  struct tme_dfb_display *display;
  int scale_old;
  int rc;

  /* return now if the menu item isn't active: */
  if (!dfb_check_menu_item_get_active(DFB_CHECK_MENU_ITEM(DFB_MENU_ITEM(widget)))) {
    return;
  }

  /* get the display: */
  display = screen->tme_dfb_screen_display;

  /* lock our mutex: */
  tme_mutex_lock(&display->tme_dfb_display_mutex);

  /* get the old scaling and set the new scaling: */
  scale_old = screen->tme_dfb_screen_fb_scale;
  if (scale_old < 0
      && scale_new < 0) {
    scale_new = scale_old;
  }
  screen->tme_dfb_screen_fb_scale = scale_new;

  /* unlock our mutex: */
  tme_mutex_unlock(&display->tme_dfb_display_mutex);

  /* call the mode change function if the scaling has changed: */
  if (scale_new != scale_old) {
    rc = _tme_dfb_screen_mode_change(screen->tme_dfb_screen_fb);
    assert (rc == TME_OK);
  }
}

/* this sets the screen scaling to default: */
static void
_tme_dfb_screen_scale_default(DfbWidget *widget,
			      struct tme_dfb_screen *screen)
{
  _tme_dfb_screen_scale_set(widget,
			    screen,
			    -TME_FB_XLAT_SCALE_NONE);
}

/* this sets the screen scaling to half: */
static void
_tme_dfb_screen_scale_half(DfbWidget *widget,
			   struct tme_dfb_screen *screen)
{
  _tme_dfb_screen_scale_set(widget,
			    screen,
			    TME_FB_XLAT_SCALE_HALF);
}

/* this sets the screen scaling to none: */
static void
_tme_dfb_screen_scale_none(DfbWidget *widget,
			   struct tme_dfb_screen *screen)
{
  _tme_dfb_screen_scale_set(widget,
			    screen,
			    TME_FB_XLAT_SCALE_NONE);
}

/* this sets the screen scaling to double: */
static void
_tme_dfb_screen_scale_double(DfbWidget *widget,
			     struct tme_dfb_screen *screen)
{
  _tme_dfb_screen_scale_set(widget,
			    screen,
			    TME_FB_XLAT_SCALE_DOUBLE);
}

/* this creates the Screen scaling submenu: */
static GCallback
_tme_dfb_screen_submenu_scaling(void *_screen,
				struct tme_dfb_display_menu_item *menu_item)
{
  struct tme_dfb_screen *screen;

  screen = (struct tme_dfb_screen *) _screen;
  menu_item->tme_dfb_display_menu_item_widget = NULL;
  switch (menu_item->tme_dfb_display_menu_item_which) {
  case 0:
    menu_item->tme_dfb_display_menu_item_string = _("Default");
    menu_item->tme_dfb_display_menu_item_widget = &screen->tme_dfb_screen_scale_default;
    return (G_CALLBACK(_tme_dfb_screen_scale_default));
  case 1:
    menu_item->tme_dfb_display_menu_item_string = _("Half");
    menu_item->tme_dfb_display_menu_item_widget = &screen->tme_dfb_screen_scale_half;
    return (G_CALLBACK(_tme_dfb_screen_scale_half));
  case 2:
    menu_item->tme_dfb_display_menu_item_string = _("Full");
    return (G_CALLBACK(_tme_dfb_screen_scale_none));
  case 3:
    menu_item->tme_dfb_display_menu_item_string = _("Double");
    return (G_CALLBACK(_tme_dfb_screen_scale_double));
  default:
    break;
  }
  return (NULL);
}

/* this makes a new screen: */
struct tme_dfb_screen *
_tme_dfb_screen_new(struct tme_dfb_display *display)
{
  struct tme_dfb_screen *screen, **_prev;
  GdkDisplay *gdkdisplay;
  GdkDeviceManager *devices;
  DfbWidget *menu_bar;
  DfbWidget *menu;
  DfbWidget *submenu;
  DfbWidget *menu_item;
  tme_uint8_t *bitmap_data;
  unsigned int y;
  cairo_surface_t *surface;
#define BLANK_SIDE (16 * 8)

  /* create the new screen and link it in: */
  for (_prev = &display->tme_dfb_display_screens;
       (screen = *_prev) != NULL;
       _prev = &screen->tme_dfb_screen_next);
  screen = *_prev = tme_new0(struct tme_dfb_screen, 1);

  /* the backpointer to the display: */
  screen->tme_dfb_screen_display = display;
  
  gdkdisplay = gdk_display_get_default();

  devices = gdk_display_get_device_manager(gdkdisplay);

  screen->tme_dfb_screen_pointer = gdk_device_manager_get_client_pointer(devices);

  /* there is no framebuffer connection yet: */
  screen->tme_dfb_screen_fb = NULL;

  /* the user hasn't specified a scaling yet: */
  screen->tme_dfb_screen_fb_scale
    = -TME_FB_XLAT_SCALE_NONE;

  /* we have no colorset: */
  screen->tme_dfb_screen_colorset = TME_FB_COLORSET_NONE;

  /* create the top-level window, and allow it to shrink, grow,
     and auto-shrink: */
  screen->tme_dfb_screen_window
    = dfb_window_new(DFB_WINDOW_TOPLEVEL);
  dfb_window_set_resizable(DFB_WINDOW(screen->tme_dfb_screen_window), FALSE);

  /* create the outer vertical packing box: */
  screen->tme_dfb_screen_vbox0
    = dfb_box_new(DFB_ORIENTATION_VERTICAL, 0);

  /* add the outer vertical packing box to the window: */
  dfb_container_add(DFB_CONTAINER(screen->tme_dfb_screen_window),
		    screen->tme_dfb_screen_vbox0);

  /* create the menu bar and pack it into the outer vertical packing
     box: */
  menu_bar = dfb_menu_bar_new ();
  dfb_box_pack_start(DFB_BOX(screen->tme_dfb_screen_vbox0), 
		     menu_bar,
		     FALSE, FALSE, 0);
  dfb_widget_show(menu_bar);

  /* create the Screen menu: */
  menu = dfb_menu_new();

  /* create the Screen scaling submenu: */
  submenu
    = _tme_dfb_display_menu_radio(screen,
				  _tme_dfb_screen_submenu_scaling);

  /* create the Screen scaling submenu item: */
  menu_item = dfb_menu_item_new_with_label(_("Scale"));
  dfb_widget_show(menu_item);
  dfb_menu_item_set_submenu(DFB_MENU_ITEM(menu_item), submenu);
  dfb_menu_shell_append(DFB_MENU_SHELL(menu), menu_item);

  /* create the Screen menu bar item, attach the menu to it, and 
     attach the menu bar item to the menu bar: */
  menu_item = dfb_menu_item_new_with_label("Screen");
  dfb_widget_show(menu_item);
  dfb_menu_item_set_submenu(DFB_MENU_ITEM(menu_item), menu);
  dfb_menu_shell_append(DFB_MENU_SHELL(menu_bar), menu_item);

  /* create an event box for the framebuffer area: */
  screen->tme_dfb_screen_event_box
    = dfb_event_box_new();

  /* pack the event box into the outer vertical packing box: */
  dfb_box_pack_start(DFB_BOX(screen->tme_dfb_screen_vbox0), 
		     screen->tme_dfb_screen_event_box,
		     FALSE, FALSE, 0);

  /* show the event box: */
  dfb_widget_show(screen->tme_dfb_screen_event_box);

  /* create a GdkPixbuf of an alternating-bits area.  we must use
     malloc() here since this memory will end up as part of an XImage,
     and X will call free() on it: */
  bitmap_data = (tme_uint8_t *)
    malloc((BLANK_SIDE * BLANK_SIDE) / 8);
  assert(bitmap_data != NULL);
  for (y = 0;
       y < BLANK_SIDE;
       y++) {
    memset(bitmap_data
	   + (y * BLANK_SIDE / 8),
	   (y & 1
	    ? 0x33
	    : 0xcc),
	   (BLANK_SIDE / 8));
  }

  surface
    = cairo_image_surface_create_for_data(bitmap_data, 
					  CAIRO_FORMAT_A1, 
					  BLANK_SIDE, 
					  BLANK_SIDE, 
					  0);

  screen->tme_dfb_screen_gdkpixbuf
    = gdk_pixbuf_get_from_surface(surface,
				  0, 0,
				  BLANK_SIDE, BLANK_SIDE);

  cairo_surface_destroy(surface);

  /* create the DfbImage for the framebuffer area: */
  screen->tme_dfb_screen_dfbimage 
    = dfb_image_new_from_pixbuf(screen->tme_dfb_screen_gdkpixbuf);

  /* Turn off double-buffering to save rendering time (since it's done already) */
  dfb_widget_set_double_buffered(screen->tme_dfb_screen_dfbimage, FALSE);

  /* add the DfbImage to the event box: */
  dfb_container_add(DFB_CONTAINER(screen->tme_dfb_screen_event_box), 
		    screen->tme_dfb_screen_dfbimage);

  /* show the DfbImage: */
  dfb_widget_show(screen->tme_dfb_screen_dfbimage);

  /* show the outer vertical packing box: */
  dfb_widget_show(screen->tme_dfb_screen_vbox0);

  /* show the top-level window: */
  dfb_widget_show(screen->tme_dfb_screen_window);

  /* there is no translation function: */
  screen->tme_dfb_screen_fb_xlat = NULL;

  /* attach the mouse to this screen: */
  _tme_dfb_mouse_attach(screen);

  /* attach the keyboard to this screen: */
  _tme_dfb_keyboard_attach(screen);

  return (screen);
}

/* this breaks a framebuffer connection: */
static int
_tme_dfb_screen_connection_break(struct tme_connection *conn, unsigned int state)
{
  abort();
}

/* this makes a new framebuffer connection: */
static int
_tme_dfb_screen_connection_make(struct tme_connection *conn,
				unsigned int state)
{
  struct tme_dfb_display *display;
  struct tme_dfb_screen *screen;
  struct tme_fb_connection *conn_fb;
  struct tme_fb_connection *conn_fb_other;

  /* recover our data structures: */
  display = (struct tme_dfb_display *) conn->tme_connection_element->tme_element_private;
  conn_fb = (struct tme_fb_connection *) conn;
  conn_fb_other = (struct tme_fb_connection *) conn->tme_connection_other;

  /* both sides must be framebuffer connections: */
  assert(conn->tme_connection_type
	 == TME_CONNECTION_FRAMEBUFFER);
  assert(conn->tme_connection_other->tme_connection_type
	 == TME_CONNECTION_FRAMEBUFFER);

  /* we're always set up to answer calls across the connection, so we
     only have to do work when the connection has gone full, namely
     taking the other side of the connection: */
  if (state == TME_CONNECTION_FULL) {

    /* lock our mutex: */
    tme_mutex_lock(&display->tme_dfb_display_mutex);

    /* if our initial screen is already connected, make a new screen: */
    screen = display->tme_dfb_display_screens;
    if (screen->tme_dfb_screen_fb != NULL) {
      screen = _tme_dfb_screen_new(display);
    }

    /* save our connection: */
    screen->tme_dfb_screen_fb = conn_fb;

    /* unlock our mutex: */
    tme_mutex_unlock(&display->tme_dfb_display_mutex);

    /* call our mode change function: */
    _tme_dfb_screen_mode_change(conn_fb);
  }

  return (TME_OK);
}

/* this makes a new connection side for a DFB screen: */
int
_tme_dfb_screen_connections_new(struct tme_dfb_display *display, 
				struct tme_connection **_conns)
{
  struct tme_fb_connection *conn_fb;
  struct tme_connection *conn;

  /* allocate a new framebuffer connection: */
  conn_fb = tme_new0(struct tme_fb_connection, 1);
  conn = &conn_fb->tme_fb_connection;
  
  /* fill in the generic connection: */
  conn->tme_connection_next = *_conns;
  conn->tme_connection_type = TME_CONNECTION_FRAMEBUFFER;
  conn->tme_connection_score = tme_fb_connection_score;
  conn->tme_connection_make = _tme_dfb_screen_connection_make;
  conn->tme_connection_break = _tme_dfb_screen_connection_break;

  /* fill in the framebuffer connection: */
  conn_fb->tme_fb_connection_mode_change = _tme_dfb_screen_mode_change;

  /* return the connection side possibility: */
  *_conns = conn;

  /* done: */
  return (TME_OK);
}