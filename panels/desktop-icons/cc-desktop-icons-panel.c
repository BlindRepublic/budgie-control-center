/* cc-desktop-icons-panel.c
 *
 * Copyright 2023 Elliot <BlindRepublic@mailo.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <shell/cc-shell.h>
#include <shell/cc-application.h>
#include <shell/cc-object-storage.h>

#include <handy.h>

#include "cc-desktop-icons-panel.h"
#include "cc-desktop-icons-resources.h"

typedef enum {
  DESKTOP_TYPE_NONE,
  DESKTOP_TYPE_BUDGIE,
  DESKTOP_TYPE_DESKTOPFOLDER,
  DESKTOP_TYPE_NEMO
} DesktopType;

struct _CcDesktopIconsPanel {
	CcPanel  parent_instance;

  GtkSwitch *enable_switch;
  GtkBox    *header_box;

  HdyPreferencesGroup *directories_group;
  HdyPreferencesGroup *icons_group;

  GtkSwitch *home_switch;
  GtkSwitch *trash_switch;
  GtkSwitch *mounts_switch;

  GtkComboBoxText *click_policy_combo;
  GtkComboBoxText *icon_size_combo;

  DesktopType desktop_type;
  GSettings  *settings;
};

CC_PANEL_REGISTER (CcDesktopIconsPanel, cc_desktop_icons_panel)

static void
cc_desktop_icons_panel_constructed (GObject *object)
{
  CcDesktopIconsPanel *panel = CC_DESKTOP_ICONS_PANEL (object);
  gchar               *show_setting;

  G_OBJECT_CLASS (cc_desktop_icons_panel_parent_class)->constructed (object);

  if (panel->desktop_type == DESKTOP_TYPE_BUDGIE)
    show_setting = "show";

  else if (panel->desktop_type == DESKTOP_TYPE_DESKTOPFOLDER)
    show_setting = "show-desktopfolder";

  else if (panel->desktop_type == DESKTOP_TYPE_NEMO)
    show_setting = "show-desktop-icons";

  else return;

  g_settings_bind (panel->settings,
                   show_setting,
                   panel->enable_switch,
                   "active",
                   G_SETTINGS_BIND_DEFAULT);

  g_object_bind_property (panel->enable_switch,
                          "active",
                          panel,
                          "sensitive",
                          G_BINDING_SYNC_CREATE);


  cc_shell_embed_widget_in_header (cc_panel_get_shell (CC_PANEL (panel)),
  									               GTK_WIDGET (panel->header_box), GTK_POS_RIGHT);
}

static void
cc_desktop_icons_panel_finalize (GObject *object)
{
  CcDesktopIconsPanel *panel = CC_DESKTOP_ICONS_PANEL (object);

  g_clear_object (&panel->settings);

  G_OBJECT_CLASS (cc_desktop_icons_panel_parent_class)->finalize (object);
}

static void
cc_desktop_icons_panel_class_init (CcDesktopIconsPanelClass *klass)
{
  GObjectClass *object_class   = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  CcPanelClass *panel_class    = CC_PANEL_CLASS (klass);

  object_class->constructed = cc_desktop_icons_panel_constructed;
  object_class->finalize    = cc_desktop_icons_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/control-center/desktop-icons/cc-desktop-icons-panel.ui");

  gtk_widget_class_bind_template_child (widget_class, CcDesktopIconsPanel, enable_switch);
  gtk_widget_class_bind_template_child (widget_class, CcDesktopIconsPanel, header_box);
  gtk_widget_class_bind_template_child (widget_class, CcDesktopIconsPanel, directories_group);
  gtk_widget_class_bind_template_child (widget_class, CcDesktopIconsPanel, icons_group);
  gtk_widget_class_bind_template_child (widget_class, CcDesktopIconsPanel, home_switch);
  gtk_widget_class_bind_template_child (widget_class, CcDesktopIconsPanel, trash_switch);
  gtk_widget_class_bind_template_child (widget_class, CcDesktopIconsPanel, mounts_switch);
  gtk_widget_class_bind_template_child (widget_class, CcDesktopIconsPanel, click_policy_combo);
  gtk_widget_class_bind_template_child (widget_class, CcDesktopIconsPanel, icon_size_combo);
}

static DesktopType
get_desktop_type (void) {
  DesktopType desktop_type;
  GSettings  *wm_settings;
  gchar      *path;

  wm_settings = g_settings_new ("com.solus-project.budgie-wm");
  desktop_type = g_settings_get_enum (wm_settings, "desktop-type-override");

  g_object_unref (wm_settings);
  if (desktop_type != DESKTOP_TYPE_NONE && desktop_type != DESKTOP_TYPE_BUDGIE)
    {
      gchar *program = desktop_type == DESKTOP_TYPE_DESKTOPFOLDER ? "com.github.spheras.desktopfolder" : "nemo-desktop";

      if ((path = g_find_program_in_path (program)))
        {
          g_free (path);
          return desktop_type;
        }
    }

  if ((path = g_find_program_in_path ("org.buddiesofbudgie.budgie-desktop-view")))
    {
      g_free (path);
      return DESKTOP_TYPE_BUDGIE;
    }

  return DESKTOP_TYPE_NONE;
}

static void
cc_desktop_icons_panel_init (CcDesktopIconsPanel *panel)
{

  g_resources_register (cc_desktop_icons_get_resource ());
  gtk_widget_init_template (GTK_WIDGET (panel));

  panel->desktop_type = get_desktop_type ();

  if (panel->desktop_type == DESKTOP_TYPE_BUDGIE)
    {
      panel->settings = g_settings_new ("org.buddiesofbudgie.budgie-desktop-view");

      g_settings_bind (panel->settings,
                       "show-home-folder",
                       panel->home_switch,
                       "active",
                       G_SETTINGS_BIND_DEFAULT);

      g_settings_bind (panel->settings,
                       "show-trash-folder",
                       panel->trash_switch,
                       "active",
                       G_SETTINGS_BIND_DEFAULT);

      g_settings_bind (panel->settings,
                       "show-active-mounts",
                       panel->mounts_switch,
                       "active",
                       G_SETTINGS_BIND_DEFAULT);

      g_settings_bind (panel->settings,
                       "click-policy",
                       panel->click_policy_combo,
                       "active-id",
                       G_SETTINGS_BIND_DEFAULT);

      g_settings_bind (panel->settings,
                       "icon-size",
                       panel->icon_size_combo,
                       "active-id",
                       G_SETTINGS_BIND_DEFAULT);

    }
  else 
    {
      gtk_widget_hide (GTK_WIDGET (panel->icons_group));

      if (panel->desktop_type == DESKTOP_TYPE_NEMO)
        {
          panel->settings = g_settings_new ("org.nemo.desktop");

          g_settings_bind (panel->settings,
                          "home-icon-visible",
                          panel->home_switch,
                          "active",
                          G_SETTINGS_BIND_DEFAULT);

          g_settings_bind (panel->settings,
                           "trash-icon-visible",
                           panel->trash_switch,
                           "active",
                           G_SETTINGS_BIND_DEFAULT);

          g_settings_bind (panel->settings,
                           "volumes-visible",
                           panel->mounts_switch,
                           "active",
                           G_SETTINGS_BIND_DEFAULT);
        }
      else
        {
          gtk_widget_hide (GTK_WIDGET (panel->directories_group));

          if (panel->desktop_type == DESKTOP_TYPE_DESKTOPFOLDER)
            panel->settings = g_settings_new ("com.github.spheras.desktopfolder");

          else
            gtk_widget_set_sensitive (panel->enable_switch, FALSE);
        }
    }
}

void
cc_desktop_icons_panel_static_init_func (void)
{
  if (get_desktop_type () == DESKTOP_TYPE_NONE)
    cc_shell_model_set_panel_visibility (cc_application_get_model (CC_APPLICATION (g_application_get_default ())),
                                         "desktop-icons", FALSE);
}