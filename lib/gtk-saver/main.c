#include <gtk/gtk.h>
#include <stdio.h>

int status = 1;
static void on_save_button_clicked(gpointer parent_window) {
  GtkWidget *dialog;
  gint response;

  dialog = gtk_file_chooser_dialog_new(
      "Save File", GTK_WINDOW(parent_window), GTK_FILE_CHOOSER_ACTION_SAVE,
      "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);

  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
                                                 TRUE);

  response = gtk_dialog_run(GTK_DIALOG(dialog));

  if (response == GTK_RESPONSE_ACCEPT) {
    gchar *filename;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    filename = gtk_file_chooser_get_filename(chooser);
    printf("%s\n", filename);
    g_free(filename);
    status = 0;
  }

  gtk_widget_destroy(dialog);
}

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window;
  window = gtk_application_window_new(app);
  on_save_button_clicked(window);
  g_application_quit(app);
}

int main(int argc, char **argv) {
  GtkApplication *app;

  app = gtk_application_new("cat.liz3.gtk-file-save", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}