#include "Pages/MainPage.h"
#include "Pages/Sidebar.h"
#include "Pages/Topbar.h"
#include <gtk/gtk.h>

typedef struct {
  GtkWidget *window;
  TopBarWidget *top_bar;
  SideBarWidget *side_bar;
  MainPageWidget *main_page;
  GtkCssProvider *css_provider;
} AppContext;

// Catppuccin Mocha theme colors [ChatGPT generated]
static const char *CATPPUCCIN_CSS =
    "window {"
    "  background-color: #1e1e2e;" /* Base */
    "  color: #cdd6f4;"            /* Text */
    "  font-family: 'SF Pro Display', -apple-system, sans-serif;"
    "  font-size: 13px;"
    "}"
    "entry {"
    "  background-color: #313244;" /* Surface0 */
    "  color: #cdd6f4;"            /* Text */
    "  border: 1px solid #45475a;" /* Surface1 */
    "  border-radius: 6px;"
    "  padding: 8px 12px;"
    "  min-height: 32px;"
    "}"
    "entry:focus {"
    "  border-color: #89b4fa;" /* Blue */
    "  box-shadow: 0 0 0 2px rgba(137, 180, 250, 0.2);"
    "}"
    "entry selection {"
    "  background-color: #89b4fa;" /* Blue */
    "  color: #1e1e2e;"            /* Base */
    "}"
    "list, listview {"
    "  background-color: #1e1e2e;" /* Base */
    "  border: none;"
    "}"
    "list > row, listview > row {"
    "  background-color: transparent;"
    "  color: #cdd6f4;" /* Text */
    "  padding: 8px 12px;"
    "  border-radius: 6px;"
    "  margin: 2px 4px;"
    "}"
    "list > row:hover, listview > row:hover {"
    "  background-color: #313244;" /* Surface0 */
    "}"
    "list > row:selected, listview > row:selected {"
    "  background-color: #89b4fa;" /* Blue */
    "  color: #1e1e2e;"            /* Base */
    "}"
    "label {"
    "  color: #cdd6f4;" /* Text */
    "  background-color: transparent;"
    "}"
    "separator {"
    "  background-color: #45475a;" /* Surface1 */
    "  min-width: 1px;"
    "  min-height: 1px;"
    "}"
    "paned > separator {"
    "  background-color: #45475a;" /* Surface1 */
    "  min-width: 4px;"
    "  min-height: 4px;"
    "}"
    "button {"
    "  background-color: #313244;" /* Surface0 */
    "  color: #cdd6f4;"            /* Text */
    "  border: 1px solid #45475a;" /* Surface1 */
    "  border-radius: 6px;"
    "  padding: 6px 12px;"
    "}"
    "button:hover {"
    "  background-color: #45475a;" /* Surface1 */
    "}"
    "button:active {"
    "  background-color: #585b70;" /* Surface2 */
    "}";

static void apply_theme(AppContext *ctx) {
  ctx->css_provider = gtk_css_provider_new();

  GError *error = NULL;
  gtk_css_provider_load_from_data(ctx->css_provider, // css_provider
                                  CATPPUCCIN_CSS,    // data
                                  -1,                // length
                                  &error);           // error

  if (error) {
    g_printerr("Failed to load CSS: %s\n", error->message);
    g_error_free(error);
    return;
  }

  gtk_style_context_add_provider_for_screen(
      gdk_screen_get_default(),                 // screen
      GTK_STYLE_PROVIDER(ctx->css_provider),    // provider
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION); // priority
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
  AppContext *ctx = (AppContext *)user_data;

  // Cleanup in reverse order of creation
  if (ctx->css_provider) {
    g_object_unref(ctx->css_provider);
  }

  MainPage_destroy(ctx->main_page);
  TopBar_destroy(ctx->top_bar);
  SideBar_destroy(ctx->side_bar);

  g_free(ctx);
}

static void activate(GtkApplication *app, gpointer user_data) {
  // Allocate application context
  AppContext *ctx = g_new0(AppContext, 1);

  // Apply theme first
  apply_theme(ctx);

  // Create window
  ctx->window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(ctx->window), "Explorer");
  gtk_window_set_default_size(GTK_WINDOW(ctx->window), // window
                              800,                     // width
                              600);                    // height

  // Create main layout
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, // orientation
                                0);                       // spacing

  // Create TopBar
  const char *initial_dir = g_get_home_dir();
  ctx->top_bar = TopBar_new(initial_dir);
  gtk_box_pack_start(GTK_BOX(vbox),          // box
                     ctx->top_bar->m_TopBar, // child
                     FALSE,                  // expand
                     FALSE,                  // fill
                     5);                     // padding

  // Create horizontal paned for sidebar and main content
  GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start(GTK_BOX(vbox), // box
                     hpaned,        // child
                     TRUE,          // expand
                     TRUE,          // fill
                     0);            // padding

  // Create SideBar
  ctx->side_bar = SideBar_new();
  gtk_paned_pack1(GTK_PANED(hpaned),        // paned
                  ctx->side_bar->m_SideBar, // child
                  FALSE,                    // resize
                  FALSE);                   // shrink
  gtk_paned_set_position(GTK_PANED(hpaned), 200);

  // Create MainPage (connects signals internally)
  ctx->main_page = MainPage_new(ctx->top_bar, ctx->side_bar);
  gtk_paned_pack2(GTK_PANED(hpaned),          // paned
                  ctx->main_page->m_MainPage, // child
                  TRUE,                       // resize
                  FALSE);                     // shrink

  // Add to window
  gtk_container_add(GTK_CONTAINER(ctx->window), vbox);

  // Connect cleanup
  g_signal_connect(ctx->window,                   // instance
                   "destroy",                     // detailed_signal
                   G_CALLBACK(on_window_destroy), // c_handler
                   ctx);                          // data

  gtk_widget_show_all(ctx->window);
}

int main(int argc, char **argv) {
  GtkApplication *app =
      gtk_application_new("com.example.explorer",    // application_id
                          G_APPLICATION_FLAGS_NONE); // flags

  g_signal_connect(app,                  // instance
                   "activate",           // detailed_signal
                   G_CALLBACK(activate), // c_handler
                   NULL);                // data

  int status = g_application_run(G_APPLICATION(app), // application
                                 argc,               // argc
                                 argv);              // argv

  g_object_unref(app);

  return status;
}
