/*  mp3asm: an mp3 frameeditor.
 *
 *  mp3asm.c: something should hold int main :)
 *
 *  Copyright (C) 2001  Luc Verhaegen (_Death_@Undernet(IRC))
 *                                    <dw_death_@hotmail.com>
 *  Copyright (C) 1996-1997 Olli Fromme <olli@fromme.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <gtk/gtk.h>
#include "menu.h"
#include "fe.h"
/*#include "mp3asm.h"*/

/* fe.c */
extern void fe_exit (void);
extern void fe_file_open_dialog (void);

/*
 * file menu: 
 *
 */

static void
menu_file_open (void)
{
  fe_file_open_dialog ();
}

static void
menu_file_save (void)
{
  fprintf (stderr, "file_save not implemented yet.");
}

static void
menu_file_save_as (void)
{
  fprintf (stderr, "file_save_as not implemented yet.");
}

static void
menu_file_close (void)
{
  fprintf (stderr, "file_close not implemented yet.");
}

static void
menu_file_quit (void)
{

  fe_exit ();
}

/*
 * edit menu: 
 *
 */

static void
menu_edit_undo (void)
{
  fprintf (stderr, "edit_undo not implemented yet.");
}

static void
menu_edit_redo (void)
{
  fprintf (stderr, "edit_redo not implemented yet.");
}

static void
menu_edit_cut (void)
{
  fprintf (stderr, "edit_cut not implemented yet.");
}

static void
menu_edit_copy (void)
{
  fprintf (stderr, "edit_copy not implemented yet.");
}

static void
menu_edit_paste (void)
{
  fprintf (stderr, "edit_paste not implemented yet.");
}

static void
menu_edit_preferences (void)
{
  fprintf (stderr, "edit_preferences not implemented yet.");
}

/*
 * log menu: 
 *
 */

static void
menu_log_show (void)
{
  fprintf (stderr, "log_show not implemented yet.");
}

static void
menu_log_save (void)
{
  fprintf (stderr, "log_save not implemented yet.");
}

static void
menu_log_save_as (void)
{
  fprintf (stderr, "log_save_as not implemented yet.");
}

static void
menu_log_flush (void)
{
  fprintf (stderr, "log_flush not implemented yet.");
}

/*
 * help menu: 
 *
 */

static void
menu_help_about (void)
{
  fprintf (stderr, "help_about not implemented yet.");
}

/*
 * menu: gtk menu nicked from xchat
 *
 */

static menu_t mymenu[] =
{
   {M_NEWMENU, "File", 0, 0, 1},
   {M_MENU, "Open", (menucallback) menu_file_open, 0, 1},
   {M_SEP, 0, 0, 0, 0},
   {M_MENU, "Save", (menucallback) menu_file_save, 0, 1},
   {M_MENU, "Save as", (menucallback) menu_file_save_as, 0, 1},
   {M_SEP, 0, 0, 0, 0},
   {M_MENU, "Close", (menucallback) menu_file_close, 0, 1},
   {M_SEP, 0, 0, 0, 0},
   {M_MENU, "Quit", (menucallback) menu_file_quit, 0, 1},  /* 09 */

   {M_NEWMENU, "Edit", 0, 0, 1},
   {M_MENU, "Undo", (menucallback) menu_edit_undo, 0, 1},
   {M_MENU, "Redo", (menucallback) menu_edit_redo, 0, 1},
   {M_SEP, 0, 0, 0, 0},
   {M_MENU, "Cut", (menucallback) menu_edit_cut, 0, 1},
   {M_MENU, "Copy", (menucallback) menu_edit_copy, 0, 1},
   {M_MENU, "Paste", (menucallback) menu_edit_paste, 0, 1},
   {M_SEP, 0, 0, 0, 0},
   {M_MENU, "Preferences", (menucallback)menu_edit_preferences, 0, 1}, /* 18 */

   {M_NEWMENU, "Log", 0, 0, 1},
   {M_MENUTOG, "Show/hide", (menucallback) menu_log_show, 1, 1},
   {M_SEP, 0, 0, 0, 0},
   {M_MENU, "Save log", (menucallback) menu_log_save, 0, 1},
   {M_MENU, "Save log as", (menucallback) menu_log_save_as, 0, 1},
   {M_SEP, 0, 0, 0, 0},
   {M_MENU, "Flush log", (menucallback) menu_log_flush, 0, 1},

   {M_NEWMENURIGHT, "Help", 0, 0, 1},
   {M_MENU, "About Mp3-ASM..", (menucallback) menu_help_about, 0, 1},

   {M_END, 0, 0, 0, 0},
};


GtkWidget *
create_menu (void)
{
   int i = 0;
   GtkWidget *item;
   GtkWidget *menu = 0;
   GtkWidget *menu_item = 0;
   GtkWidget *menu_bar = gtk_menu_bar_new ();
   GtkWidget *usermenu = 0;
   GtkWidget *tearoff = 0;
   mymenu[20].state = 1;

   while (1)
   {
      switch (mymenu[i].type)
      {
      case M_NEWMENURIGHT:
      case M_NEWMENU:
         if (menu)
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
         menu = gtk_menu_new ();
         if (mymenu[i].callback == (void *)-1)
            usermenu = menu;
         menu_item = gtk_menu_item_new_with_label (mymenu[i].text);
         if (mymenu[i].type == M_NEWMENURIGHT)
            gtk_menu_item_right_justify ((GtkMenuItem *) menu_item);
	 tearoff = gtk_tearoff_menu_item_new ();
	 gtk_menu_append (GTK_MENU (menu), tearoff);
	 gtk_widget_show (tearoff);
         gtk_menu_bar_append (GTK_MENU_BAR (menu_bar), menu_item);
         gtk_widget_show (menu_item);
         break;
      case M_MENU:
         item = gtk_menu_item_new_with_label (mymenu[i].text);
         if (mymenu[i].callback)
            gtk_signal_connect_object (GTK_OBJECT (item), "activate", GTK_SIGNAL_FUNC (mymenu[i].callback), NULL);
         gtk_menu_append (GTK_MENU (menu), item);
         gtk_widget_show (item);
         gtk_widget_set_sensitive (item, mymenu[i].activate);
         break;
      case M_MENUTOG:
         item = gtk_check_menu_item_new_with_label (mymenu[i].text);
         gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (item), mymenu[i].state);
         if (mymenu[i].callback)
            gtk_signal_connect_object (GTK_OBJECT (item), "toggled", GTK_SIGNAL_FUNC (mymenu[i].callback), NULL);
         gtk_menu_append (GTK_MENU (menu), item);
         gtk_widget_show (item);
         gtk_widget_set_sensitive (item, mymenu[i].activate);
         break;
      case M_SEP:
         item = gtk_menu_item_new ();
         gtk_widget_set_sensitive (item, FALSE);
         gtk_menu_append (GTK_MENU (menu), item);
         gtk_widget_show (item);
         break;
      case M_END:
	if (menu)
	  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
	/* if (usermenu)
	   usermenu_create (usermenu);
	   gui->menu = usermenu;*/
	return (menu_bar);
      }
      i++;
   }
}
