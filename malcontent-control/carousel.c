/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright © 2016 Red Hat, Inc.
 * Copyright © 2019 Endless Mobile, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  - Felipe Borges <felipeborges@gnome.org>
 *  - Philip Withnall <withnall@endlessm.com>
 */

#include <glib-object.h>
#include <gtk/gtk.h>

#include "carousel.h"


#define ARROW_SIZE 20

struct _MctCarouselItem {
  GtkRadioButton parent;

  gint page;
};

G_DEFINE_TYPE (MctCarouselItem, mct_carousel_item, GTK_TYPE_RADIO_BUTTON)

GtkWidget *
mct_carousel_item_new (void)
{
  return g_object_new (MCT_TYPE_CAROUSEL_ITEM, NULL);
}

static void
mct_carousel_item_class_init (MctCarouselItemClass *klass)
{
}

static void
mct_carousel_item_init (MctCarouselItem *self)
{
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (self), FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)),
             "carousel-item");
}

struct _MctCarousel {
  GtkRevealer parent;

  GList *children;
  gint visible_page;
  MctCarouselItem *selected_item;
  GtkWidget *last_box;
  GtkWidget *arrow;
  gint arrow_start_x;

  /* Widgets */
  GtkStack *stack;
  GtkWidget *go_back_button;
  GtkWidget *go_next_button;

  GtkStyleProvider *provider;
};

G_DEFINE_TYPE (MctCarousel, mct_carousel, GTK_TYPE_REVEALER)

enum {
  ITEM_ACTIVATED,
  NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0, };

#define ITEMS_PER_PAGE 3

static gint
mct_carousel_item_get_x (MctCarouselItem *item,
                         MctCarousel     *carousel)
{
  GtkWidget *widget, *parent;
  gint width;
  gint dest_x;

  parent = GTK_WIDGET (carousel->stack);
  widget = GTK_WIDGET (item);

  width = gtk_widget_get_allocated_width (widget);
  if (!gtk_widget_translate_coordinates (widget,
                                         parent,
                                         width / 2,
                                         0,
                                         &dest_x,
                                         NULL))
    return 0;

  return CLAMP (dest_x - ARROW_SIZE,
                0,
                gtk_widget_get_allocated_width (parent));
}

static void
mct_carousel_move_arrow (MctCarousel *self)
{
  GtkStyleContext *context;
  gchar *css;
  gint end_x;
  GtkSettings *settings;
  gboolean animations;

  if (!self->selected_item)
    return;

  end_x = mct_carousel_item_get_x (self->selected_item, self);

  context = gtk_widget_get_style_context (self->arrow);
  if (self->provider)
    gtk_style_context_remove_provider (context, self->provider);
  g_clear_object (&self->provider);

  settings = gtk_widget_get_settings (GTK_WIDGET (self));
  g_object_get (settings, "gtk-enable-animations", &animations, NULL);

  /* Animate the arrow movement if animations are enabled. Otherwise,
   * jump the arrow to the right location instantly. */
  if (animations)
    {
      css = g_strdup_printf ("@keyframes arrow_keyframes-%d-%d {\n"
                             "  from { margin-left: %dpx; }\n"
                             "  to { margin-left: %dpx; }\n"
                             "}\n"
                             "* {\n"
                             "  animation-name: arrow_keyframes-%d-%d;\n"
                             "}\n",
                             self->arrow_start_x, end_x,
                             self->arrow_start_x, end_x,
                             self->arrow_start_x, end_x);
    }
  else
    {
      css = g_strdup_printf ("* { margin-left: %dpx }", end_x);
    }

  self->provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (self->provider), css, -1, NULL);
  gtk_style_context_add_provider (context, self->provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_free (css);
}

static gint
get_last_page_number (MctCarousel *self)
{
  if (g_list_length (self->children) == 0)
    return 0;

  return ((g_list_length (self->children) - 1) / ITEMS_PER_PAGE);
}

static void
update_buttons_visibility (MctCarousel *self)
{
  gtk_widget_set_visible (self->go_back_button, (self->visible_page > 0));
  gtk_widget_set_visible (self->go_next_button, (self->visible_page < get_last_page_number (self)));
}

/**
 * mct_carousel_find_item:
 * @carousel: an MctCarousel instance
 * @data: user data passed to the comparison function
 * @func: the function to call for each element.
 *      It should return 0 when the desired element is found
 *
 * Finds an MctCarousel item using the supplied function to find the
 * desired element.
 * Ideally useful for matching a model object and its correspondent
 * widget.
 *
 * Returns: the found MctCarouselItem, or %NULL if it is not found
 */
MctCarouselItem *
mct_carousel_find_item (MctCarousel   *self,
                        gconstpointer  data,
                        GCompareFunc   func)
{
  GList *list;

  list = self->children;
  while (list != NULL)
    {
      if (!func (list->data, data))
        return list->data;
      list = list->next;
    }

  return NULL;
}

static void
on_item_toggled (MctCarouselItem *item,
                 GdkEvent        *event,
                 gpointer         user_data)
{
  MctCarousel *self = MCT_CAROUSEL (user_data);

  mct_carousel_select_item (self, item);
}

void
mct_carousel_select_item (MctCarousel     *self,
                          MctCarouselItem *item)
{
  gchar *page_name;
  gboolean page_changed = TRUE;

  /* Select first user if none is specified */
  if (item == NULL)
    {
      if (self->children != NULL)
        item = self->children->data;
      else
        return;
    }

  if (self->selected_item != NULL)
    {
      page_changed = (self->selected_item->page != item->page);
      self->arrow_start_x = mct_carousel_item_get_x (self->selected_item, self);
    }

  self->selected_item = item;
  self->visible_page = item->page;
  g_signal_emit (self, signals[ITEM_ACTIVATED], 0, item);

  if (!page_changed)
    {
      mct_carousel_move_arrow (self);
      return;
    }

  page_name = g_strdup_printf ("%d", self->visible_page);
  gtk_stack_set_visible_child_name (self->stack, page_name);

  g_free (page_name);

  update_buttons_visibility (self);

  /* mct_carousel_move_arrow is called from on_transition_running */
}

static void
mct_carousel_select_item_at_index (MctCarousel *self,
                                   gint         index)
{
  GList *l = NULL;

  l = g_list_nth (self->children, index);
  mct_carousel_select_item (self, l->data);
}

static void
mct_carousel_goto_previous_page (GtkWidget *button,
                                 gpointer   user_data)
{
  MctCarousel *self = MCT_CAROUSEL (user_data);

  self->visible_page--;
  if (self->visible_page < 0)
    self->visible_page = 0;

  /* Select first item of the page */
  mct_carousel_select_item_at_index (self, self->visible_page * ITEMS_PER_PAGE);
}

static void
mct_carousel_goto_next_page (GtkWidget *button,
                             gpointer   user_data)
{
  MctCarousel *self = MCT_CAROUSEL (user_data);
  gint last_page;

  last_page = get_last_page_number (self);

  self->visible_page++;
  if (self->visible_page > last_page)
    self->visible_page = last_page;

  /* Select first item of the page */
  mct_carousel_select_item_at_index (self, self->visible_page * ITEMS_PER_PAGE);
}

static void
mct_carousel_add (GtkContainer *container,
                  GtkWidget    *widget)
{
  MctCarousel *self = MCT_CAROUSEL (container);
  gboolean last_box_is_full;

  if (!MCT_IS_CAROUSEL_ITEM (widget))
    {
      GTK_CONTAINER_CLASS (mct_carousel_parent_class)->add (container, widget);
      return;
    }

  gtk_style_context_add_class (gtk_widget_get_style_context (widget), "menu");
  gtk_button_set_relief (GTK_BUTTON (widget), GTK_RELIEF_NONE);

  self->children = g_list_append (self->children, widget);
  MCT_CAROUSEL_ITEM (widget)->page = get_last_page_number (self);
  if (self->selected_item != NULL)
    gtk_radio_button_join_group (GTK_RADIO_BUTTON (widget), GTK_RADIO_BUTTON (self->selected_item));
  g_signal_connect (widget, "button-press-event", G_CALLBACK (on_item_toggled), self);

  last_box_is_full = ((g_list_length (self->children) - 1) % ITEMS_PER_PAGE == 0);
  if (last_box_is_full)
    {
      g_autofree gchar *page = NULL;

      page = g_strdup_printf ("%d", MCT_CAROUSEL_ITEM (widget)->page);
      self->last_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_widget_show (self->last_box);
      gtk_widget_set_valign (self->last_box, GTK_ALIGN_CENTER);
      gtk_stack_add_named (self->stack, self->last_box, page);
    }

  gtk_widget_show_all (widget);
  gtk_box_pack_start (GTK_BOX (self->last_box), widget, TRUE, FALSE, 10);

  update_buttons_visibility (self);
}

static void
destroy_widget_cb (GtkWidget *widget,
                   gpointer   user_data)
{
  gtk_widget_destroy (widget);
}

void
mct_carousel_purge_items (MctCarousel *self)
{
  gtk_container_forall (GTK_CONTAINER (self->stack),
                        destroy_widget_cb,
                        NULL);

  g_list_free (self->children);
  self->children = NULL;
  self->visible_page = 0;
  self->selected_item = NULL;
}

MctCarousel *
mct_carousel_new (void)
{
  return g_object_new (MCT_TYPE_CAROUSEL, NULL);
}

static void
mct_carousel_dispose (GObject *object)
{
  MctCarousel *self = MCT_CAROUSEL (object);

  g_clear_object (&self->provider);
  if (self->children != NULL)
    {
      g_list_free (self->children);
      self->children = NULL;
    }

  G_OBJECT_CLASS (mct_carousel_parent_class)->dispose (object);
}

static void
mct_carousel_class_init (MctCarouselClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wclass = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  gtk_widget_class_set_template_from_resource (wclass,
                                               "/org/freedesktop/MalcontentControl/ui/carousel.ui");

  gtk_widget_class_bind_template_child (wclass, MctCarousel, stack);
  gtk_widget_class_bind_template_child (wclass, MctCarousel, go_back_button);
  gtk_widget_class_bind_template_child (wclass, MctCarousel, go_next_button);
  gtk_widget_class_bind_template_child (wclass, MctCarousel, arrow);

  gtk_widget_class_bind_template_callback (wclass, mct_carousel_goto_previous_page);
  gtk_widget_class_bind_template_callback (wclass, mct_carousel_goto_next_page);

  object_class->dispose = mct_carousel_dispose;

  container_class->add = mct_carousel_add;

  signals[ITEM_ACTIVATED] =
      g_signal_new ("item-activated",
                    MCT_TYPE_CAROUSEL,
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL, NULL,
                    g_cclosure_marshal_VOID__OBJECT,
                    G_TYPE_NONE, 1,
                    MCT_TYPE_CAROUSEL_ITEM);
}

static void
on_size_allocate (MctCarousel *self)
{
  if (self->selected_item == NULL)
    return;

  if (gtk_stack_get_transition_running (self->stack))
    return;

  self->arrow_start_x = mct_carousel_item_get_x (self->selected_item, self);
  mct_carousel_move_arrow (self);
}

static void
on_transition_running (MctCarousel *self)
{
  if (!gtk_stack_get_transition_running (self->stack))
    mct_carousel_move_arrow (self);
}

static void
mct_carousel_init (MctCarousel *self)
{
  GtkStyleProvider *provider;

  gtk_widget_init_template (GTK_WIDGET (self));

  provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_css_provider_load_from_resource (GTK_CSS_PROVIDER (provider),
                                       "/org/freedesktop/MalcontentControl/ui/carousel.css");

  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             provider,
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION - 1);

  g_object_unref (provider);

  g_signal_connect_swapped (self->stack, "size-allocate", G_CALLBACK (on_size_allocate), self);
  g_signal_connect_swapped (self->stack, "notify::transition-running", G_CALLBACK (on_transition_running), self);
}

guint
mct_carousel_get_item_count (MctCarousel *self)
{
  return g_list_length (self->children);
}
