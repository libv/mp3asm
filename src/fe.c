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
#include <stdlib.h>
#include <string.h>
#include "mp3asm.h"
#include "fe.h"

/* utils.c */
void *tmalloc (size_t size);

/* mp3asm.c */
extern int open_mp3 (char *filename);

/* menu.c */
extern GtkWidget *create_menu (void);


void fe_exit (void);

/*
 * fe_update: make sure the front end dusnt freeze up completly
 *
 */
void
fe_update (void)
{
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

/*
 * fe_simple_dialog: Shows a simple dialog with the message as label & an ok button
 *
 */
void
fe_simple_dialog (char *message)
{
  GtkWidget *dialog, *label, *ok_button;
     
  dialog = gtk_dialog_new();
  label = gtk_label_new (message);
  ok_button = gtk_button_new_with_label("Ok");
     
  gtk_signal_connect_object (GTK_OBJECT (ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) dialog);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area), ok_button);
  
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
  gtk_widget_show_all (dialog);
}

/*
 * gtk_destroy_gui_handler:
 *
 */

static int
gtk_gui_destroy_handler (GtkWidget *win, void *blah)
{
  fe_exit ();
  return TRUE;
}

/*
 * create_main_window:
 *
 */
static void
create_main_window (void) 
{
  char buf[512];

  gui->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (gui->window);

  sprintf (buf, "Mp3-ASM %s: Main Window", VERSION);
  gtk_window_set_title ((GtkWindow *) gui->window, buf);
  gtk_signal_connect ((GtkObject *) gui->window, "destroy", GTK_SIGNAL_FUNC (gtk_gui_destroy_handler), NULL);
  gtk_window_set_policy ((GtkWindow *) gui->window, TRUE, TRUE, FALSE);
  gtk_window_set_default_size ((GtkWindow *) gui->window, 200, 200);

  gui->mainbox = gtk_vbox_new (0, 0);
  gtk_container_add (GTK_CONTAINER (gui->window), gui->mainbox);
  gtk_widget_show (gui->mainbox);

  gui->menu = create_menu ();
  gtk_widget_show (gui->menu);
  gtk_box_pack_start (GTK_BOX (gui->mainbox), gui->menu, FALSE, FALSE, 0);

  gtk_widget_show (gui->window);
}

/*
 * fe_init:
 *
 */
void
fe_init (int *argc, char **argv[])
{
  gtk_init (argc, argv);

  gui = tmalloc (sizeof (gui_t));
  memset (gui, 0, sizeof (gui_t));
  create_main_window ();

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

/*
 *
 *
 */
void
gtk_mp3gui_scroll (GtkAdjustment *adjust, gpointer data)
{

}

/*
 * fe_mp3gui_show: 
 *
 */
void fe_mp3gui_show (mp3_t *mp3)
{
  mp3gui_t *mp3gui = mp3->gui = tmalloc (sizeof (mp3gui_t));
  
  mp3->gui->mbox = gtk_vbox_new (FALSE, 10);
  
  mp3gui->hruler = gtk_hruler_new ();
  gtk_ruler_set_metric (GTK_RULER (mp3gui->hruler), GTK_PIXELS);
  gtk_ruler_set_range (GTK_RULER (mp3gui->hruler), 7, 13, 0, 20);
  gtk_box_pack_start (GTK_BOX (mp3gui->mbox), mp3gui->hruler, FALSE, FALSE, 0);
  gtk_widget_show (mp3gui->hruler);

  mp3gui->area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (mp3gui->area), 150, 100);
  gtk_widget_show (mp3gui->area);
  gtk_box_pack_start (GTK_BOX (mp3gui->mbox), mp3gui->area, TRUE, TRUE, 0);
  gtk_widget_show (mp3gui->area);

  (GtkObject *) mp3gui->adjust = gtk_adjustment_new (0.0, 0.0, 10.0, 1.0, 1.0, 5.0);
  mp3gui->hscroll = gtk_hscrollbar_new (GTK_ADJUSTMENT (mp3gui->adjust));
  gtk_box_pack_start (GTK_BOX (mp3gui->mbox), mp3gui->hscroll, FALSE, FALSE, 0);
  gtk_widget_show (mp3gui->hscroll);

  gtk_signal_connect (GTK_OBJECT (mp3gui->adjust),
                      "value_changed", gtk_mp3gui_scroll,
                      GTK_OBJECT (mp3gui));

  /*gtk_signal_connect (GTK_OBJECT (mp3gui),
                      "size_allocate", gtk_mp3gui_resize_event,
                      GTK_OBJECT (mp3gui));

  gtk_signal_connect (GTK_OBJECT (mp3gui->area),
                      "realize", on_area_realize,
                      GTK_OBJECT (mp3gui));

  gtk_signal_connect (GTK_OBJECT (mp3gui->area),
                      "expose_event", on_area_expose_event,
                      GTK_OBJECT (mp3gui));

  gtk_signal_connect (GTK_OBJECT (mp3gui->area),
                      "button_press_event", GTK_SIGNAL_FUNC (gtk_wave_view_button_press_event),
                      GTK_OBJECT (mp3gui));

  gtk_signal_connect (GTK_OBJECT (mp3gui->area),
                      "button_release_event", gtk_wave_view_button_release_event,
                      GTK_OBJECT (mp3gui));

  gtk_signal_connect (GTK_OBJECT (mp3gui->area),
                      "motion_notify_event", GTK_SIGNAL_FUNC (gtk_wave_view_motion_notify_event),
                      GTK_OBJECT (mp3gui));*/

  gtk_widget_set_events (mp3gui->area, GDK_EXPOSURE_MASK |
                                         GDK_POINTER_MOTION_MASK |
                                         GDK_BUTTON_PRESS_MASK |
                                         GDK_BUTTON_RELEASE_MASK |
                                         GDK_BUTTON_MOTION_MASK |
                                         GDK_POINTER_MOTION_HINT_MASK);
}

/*
 * fe_file_open: gets filename from a file selection dialog
 *
 */
void
fe_file_open (GtkFileSelection *selector, GtkFileSelection *file_selector)
{
  /* the definition of this function cant be usefull!!! */
  char *filename = gtk_file_selection_get_filename (file_selector);
  int temp;

  while (gtk_events_pending ())
    gtk_main_iteration ();
  
  temp = open_mp3 (filename);
  
  if (temp)
    {
      char buf[512];
      sprintf (buf, "Cannot read from\n %s", filename);
      switch (temp)
	{
	case -1:
	  strcat (buf, "\nUnable to open file.");
	  break;
	case -2:
	  strcat (buf, "\nUnable to lock file.");
	  break;
	case -3:
	  strcat (buf, "\nUnable to read from file.");
	  break;
	case -4:
	  strcat (buf, "\nThis is not an mp1/2/3 file.");
	  break;
	}
      fe_simple_dialog (buf);
    }
}

/*
 * fe_file_open_dialog: 
 *
 */
void
fe_file_open_dialog (void)
{
  GtkWidget *file_selector = gtk_file_selection_new("Mp3-ASM: open file.");
  
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button), "clicked", GTK_SIGNAL_FUNC (fe_file_open), file_selector);
    
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) file_selector);
  
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) file_selector);
  
  gtk_widget_show (file_selector);
}

/*
 * fe_exit:
 *
 */
void
fe_exit (void)
{
  gtk_main_quit ();
  free(gui);
}

/*
 * fe_main:
 *
 */
void
fe_main (void)
{
  gtk_main ();
}

/* EOF */
