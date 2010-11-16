/*****************************************************************************
 * GKrellM GKfreq                                                            *
 * A plugin for GKrellM showing current cpu frequency scale                  *
 * Copyright (C) 2010 Carlo Casta <carlo.casta@gmail.com>                    *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *  
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program; if not, write to the Free Software               *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 *                                                                           *
 *****************************************************************************/

#include <gkrellm2/gkrellm.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#define GKFREQ_MAX_CPUS 8

static GkrellmMonitor *monitor;
static GkrellmPanel *panel;
static GkrellmDecal *decal_text[8];
static gint style_id;
static gint cpu_online[8];


static void read_MHz(int cpu_id, char *buffer_, size_t bufsz_)
{
	FILE *f;
	char syspath[64];
	int i;

	snprintf(syspath, 64, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", cpu_id);

	if ((f = fopen(syspath, "r")) == NULL) {
		snprintf(buffer_, bufsz_, "CPU%d N/A MHz", cpu_id);
	} else {
		fscanf(f, "%d", &i);
		if (i < 1000000)
			snprintf(buffer_, bufsz_, "CPU%d @ %d MHz", cpu_id, i/1000);
		else
			snprintf(buffer_, bufsz_, "CPU%d @ %.2f GHz", cpu_id, i * 0.000001f);

		fclose(f);
	}
}


static void get_CPUCount()
{
	char c;
	int i;

	FILE *f = fopen("/sys/devices/system/cpu/online", "r");
	if (f != NULL) {

		i = 0;
		while ((c = fgetc(f)) != EOF) {
			if (c == '-') {
				continue;
			} else if (isdigit(c)) {
				short cpu_id = c - '0';
				if (cpu_id >= 0 && cpu_id < 8) {
					cpu_online[i++] = cpu_id;
				}
			}
		}
		
		fclose(f);
	}
}


static gint panel_expose_event(GtkWidget *widget, GdkEventExpose *ev)
{
	gdk_draw_pixmap(widget->window,
	                widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
	                panel->pixmap,
	                ev->area.x,
	                ev->area.y,
	                ev->area.x,
	                ev->area.y,
	                ev->area.width,
	                ev->area.height);

	return 0;
}


static void update_plugin()
{
	static gint w, x_scroll[GKFREQ_MAX_CPUS];
	static gchar info[32];
	gint i, idx;

	w = gkrellm_chart_width();

	i = 0;
	while ((idx = cpu_online[i++]) != -1) {
		
		gboolean scrolling;
		gint w_scroll, w_decal;

		read_MHz(idx, info, 31);

		w_scroll = gdk_string_width(
			gdk_font_from_description(decal_text[idx]->text_style.font),
			info
		);

		decal_text[idx]->x_off = (w - w_scroll) / 2;
		scrolling = (decal_text[idx]->x_off < 0)? TRUE : FALSE;
		if (scrolling) {
#if defined(GKRELLM_HAVE_DECAL_SCROLL_TEXT)
			gkrellm_decal_scroll_text_set_text(panel, decal_text[idx], info);
			gkrellm_decal_scroll_text_get_size(decal_text[idx], &w_scroll, NULL);
			gkrellm_decal_get_size(decal_text[idx], &w_decal, NULL);
			
			x_scroll[idx] = (x_scroll[idx] + 1) % (2 * w);
			
			gkrellm_decal_scroll_text_horizontal_loop(decal_text[idx], scrolling);
			gkrellm_decal_text_set_offset(decal_text[idx], x_scroll[idx], 0);
#else
			decal_text[idx]->x_off = 0;
			gkrellm_draw_decal_text(panel, decal_text[idx], info, 0);
#endif
		} else {
			gkrellm_draw_decal_text(panel, decal_text[idx], info, 0);
		}
	}

	gkrellm_draw_panel_layers(panel);
}


static void create_plugin(GtkWidget *vbox, gint first_create) 
{
	GkrellmStyle *style;
	GkrellmTextstyle *ts, *ts_alt;
	gint i, idx, y;

	memset(cpu_online, -1, GKFREQ_MAX_CPUS * sizeof(gint));

	get_CPUCount();

	if (first_create)
		panel = gkrellm_panel_new0();

	style = gkrellm_meter_style(style_id);

	ts = gkrellm_meter_textstyle(style_id);
	ts_alt = gkrellm_meter_alt_textstyle(style_id);

	y = -1;
	i = 0;
	while ((idx = cpu_online[i++]) != -1) {
		decal_text[idx] = gkrellm_create_decal_text(panel, "CPU8 @ 88888 MHz", ts, style, -1, y, -1);
		y += decal_text[idx]->y + decal_text[idx]->h + style->border.top + style->border.bottom;
	}

	gkrellm_panel_configure(panel, NULL, style);
	gkrellm_panel_create(vbox, monitor, panel);

	if (first_create)
		g_signal_connect(G_OBJECT (panel->drawing_area),
		                 "expose_event",
		                 G_CALLBACK (panel_expose_event),
		                 NULL);
}


static GkrellmMonitor plugin_mon = {
	"gkfreq",                     /* Name, for config tab */
	0,                            /* Id, 0 if a plugin */
	create_plugin,                /* The create function */
	update_plugin,                /* The update function */
	NULL,                         /* The config tab create function */
	NULL,                         /* Apply the config function */

	NULL,                         /* Save user config */
	NULL,                         /* Load user config */
	NULL,                         /* config keyword */

	NULL,                         /* Undefined 2 */
	NULL,                         /* Undefined 1 */
	NULL,                         /* private */

	MON_CPU,                      /* Insert plugin before this monitor */
	NULL,                         /* Handle if a plugin, filled in by GKrellM */
	NULL                          /* path if a plugin, filled in by GKrellM */
};


GkrellmMonitor* gkrellm_init_plugin()
{
	style_id = gkrellm_add_meter_style(&plugin_mon, "gkfreq");
	monitor = &plugin_mon;

	return &plugin_mon;
}
