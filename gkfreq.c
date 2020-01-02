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

#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <gkrellm2/gkrellm.h>

/*
 * plugin can be compiled to handle maximum supported kernel cpus
 * by defining GKFREQ_MAX_CPUS as `cat /sys/devices/system/cpu/kernel_max`
 */
#ifndef GKFREQ_MAX_CPUS
 #define GKFREQ_MAX_CPUS 32
#endif

#define GKFREQ_CONFIG_KEYWORD "gkfreq"

static GkrellmMonitor *monitor;
static GkrellmPanel *panel;
static GkrellmDecal *decal_text[GKFREQ_MAX_CPUS];
static GtkWidget *text_format_combo_box;
static int style_id;
static gchar *text_format;


static void format_freq_string(int cpuid, int hz, char *buf, int buf_size)
{
	gchar *format_iter;
	char *buf_iter;
	int len = 0;

	if (buf == NULL || buf_size <= 0) {
		return;
	}
	
	if (hz < 0) {
	 	snprintf(buf, buf_size, "N/A");
		return;
	}
	
	format_iter = text_format;
	buf_iter = buf;
	while (*format_iter != '\0' && len < buf_size) {
		if (*format_iter == '$') {

			++format_iter;
			switch (*format_iter) {
			case 'L':
				len = snprintf(buf_iter, buf_size, "CPU%d", cpuid);
				break;
			case 'N':
				len = snprintf(buf_iter, buf_size, "%d", cpuid);
				break;
			case 'M':
				len = snprintf(buf_iter, buf_size, "%d MHz", hz / 1000);
				break;
			case 'm':
				len = snprintf(buf_iter, buf_size, "%d", hz / 1000);
				break;
			case 'G':
				len = snprintf(buf_iter, buf_size, "%.2f GHz", hz * 0.000001f);
				break;
			case 'g':
				len = snprintf(buf_iter, buf_size, "%.2f", hz * 0.000001f);
				break;
			default:
				*buf_iter = *format_iter;
				len = 1;
				break;
			}

		} else {

			*buf_iter = *format_iter;
			len = 1;

		}
		
		buf_size -= len;
		buf_iter += len;
		++format_iter;
	}

	*buf_iter = '\0';
}


static int is_cpu_online(int cpuid)
{
	static char syspath[64];
	snprintf(
		syspath,
		64,
		"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",
		cpuid);
	syspath[63] = '\0';

	static struct stat buffer;
	return (stat(syspath, &buffer) != 0)? -1 : 0;
}

static int read_freq(int cpuid, char *buf, int buf_size)
{
	FILE* f = NULL;
	int freq = -1;

	static char syspath[64];
	snprintf(syspath,
		64,
		"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",
		cpuid);
	syspath[63] = '\0';

	if (decal_text[cpuid] != NULL && (f = fopen(syspath, "r")) != NULL) {
		if (fscanf(f, "%d", &freq) == 1) {
    			fclose(f);

	    		format_freq_string(cpuid, freq, buf, buf_size);
		    	return 0;
        	}
	}
	
	return -1;
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
	static int w;
	static int x_scroll[GKFREQ_MAX_CPUS];
	static char info[32];
	int idx = 0;

	w = gkrellm_chart_width();

	for (idx = 0; idx < GKFREQ_MAX_CPUS; ++idx) {
		
		gboolean scrolling;
		gint w_scroll;
		gint w_decal;

		if (read_freq(idx, info, 31) == -1)
			continue;

		w_scroll = gdk_string_width(
			gdk_font_from_description(decal_text[idx]->text_style.font),
			info
		);

		decal_text[idx]->x_off = (w - w_scroll) / 2;
		scrolling = (decal_text[idx]->x_off < 0)? TRUE : FALSE;
		if (scrolling) {
#if defined(GKRELLM_HAVE_DECAL_SCROLL_TEXT)
			gkrellm_decal_scroll_text_set_text(panel, decal_text[idx], info);
			gkrellm_decal_scroll_text_get_size(decal_text[idx],
			                                   &w_scroll,
			                                   NULL);
			gkrellm_decal_get_size(decal_text[idx], &w_decal, NULL);
			
			x_scroll[idx] = (x_scroll[idx] + 1) % (2 * w);
			
			gkrellm_decal_scroll_text_horizontal_loop(decal_text[idx],
			                                          scrolling);
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


static void cb_text_format(GtkWidget *widget, gpointer data)
{
	gchar *s;
	GtkWidget *entry;

	entry = gtk_bin_get_child(GTK_BIN(text_format_combo_box));
	s = gkrellm_gtk_entry_get_text(&entry);
	gkrellm_dup_string(&text_format, s);
}


static gchar *gkfreq_info_text[] = {
	N_("<h>Label\n"),
	N_("Substitution variables for the format string for label:\n"),
	N_("\t$L    the CPU label\n"),
	N_("\t$N    the CPU id\n"),
	N_("\t$M    the CPU frequency, in MHz\n"),
	N_("\t$m    the CPU frequency, in MHz, without 'MHz' string\n"),
	N_("\t$G    the CPU frequency, in GHz\n"),
	N_("\t$g    the CPU frequency, in GHz, without 'GHz' string\n"),
	N_("\t$$    $ symbol")
};

static void create_plugin_tab(GtkWidget *tab_vbox)
{
	GtkWidget *tabs;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *vbox1;
	GtkWidget *text;
	int i;
	
	tabs = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

	vbox = gkrellm_gtk_framed_notebook_page(tabs, _("Setup"));
	vbox1 = gkrellm_gtk_category_vbox(vbox, _("Format String for Label"),
	                                  4, 0, TRUE);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	text_format_combo_box = gtk_combo_box_entry_new_text();
	gtk_box_pack_start(GTK_BOX(hbox), text_format_combo_box, TRUE, TRUE, 0);
	gtk_combo_box_append_text(GTK_COMBO_BOX(text_format_combo_box),
	                          text_format);
	gtk_combo_box_append_text(GTK_COMBO_BOX(text_format_combo_box),
	                          _("$L: $F"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(text_format_combo_box), 0);
	
	g_signal_connect(G_OBJECT(GTK_COMBO_BOX(text_format_combo_box)),
	                 "changed",
	                 G_CALLBACK(cb_text_format),
	                 NULL);

	text = gkrellm_gtk_scrolled_text_view(vbox, NULL, GTK_POLICY_AUTOMATIC,
	                                                  GTK_POLICY_AUTOMATIC);

	for (i = 0; i < sizeof(gkfreq_info_text) / sizeof(gchar *); ++i)
		gkrellm_gtk_text_view_append(text, _(gkfreq_info_text[i]));
}


static void save_plugin_config(FILE *f)
{
	fprintf(f, "%s text_format %s\n", GKFREQ_CONFIG_KEYWORD, text_format);
}


static void load_plugin_config(gchar *arg)
{
	gchar config[32];
	gchar item[CFG_BUFSIZE];
	
	if ((sscanf(arg, "%31s %[^\n]", config, item)) == 2)
		gkrellm_dup_string(&text_format, item);
	else
		gkrellm_dup_string(&text_format, "$L: $F");
}


void gkfreq_click_event(GtkWidget *w, GdkEventButton *event, gpointer p)
{
	if (event->button == 3)
		gkrellm_open_config_window(monitor);
}


static void create_plugin(GtkWidget *vbox, gint first_create) 
{
	GkrellmStyle *style;
	GkrellmTextstyle *ts;
	int idx, y;

	if (first_create)
		panel = gkrellm_panel_new0();

	style = gkrellm_meter_style(style_id);
	ts = gkrellm_meter_textstyle(style_id);

	memset(decal_text, 0, GKFREQ_MAX_CPUS * sizeof(GkrellmDecal*));

	for (y = -1, idx = 0; idx < GKFREQ_MAX_CPUS; ++idx) {
		if (is_cpu_online(idx) == 0) {
			decal_text[idx] = gkrellm_create_decal_text(panel,
			                                            "CPU8: @ 8888GHz",
			                                            ts,
			                                            style,
			                                            -1,
			                                            y,
			                                            -1);

			y = decal_text[idx]->y + decal_text[idx]->h + 1;
		}

	}
	gkrellm_panel_configure(panel, NULL, style);
	gkrellm_panel_create(vbox, monitor, panel);

	if (first_create) {
		g_signal_connect(G_OBJECT(panel->drawing_area),
		                 "expose_event",
		                 G_CALLBACK(panel_expose_event),
		                 NULL);
		
		g_signal_connect(G_OBJECT(panel->drawing_area),
		                 "button_press_event",
		                 G_CALLBACK(gkfreq_click_event),
		                 NULL);

	}
}


static GkrellmMonitor plugin_mon = {
	"gkfreq",                   /* Name, for config tab */
	0,                          /* Id, 0 if a plugin */
	create_plugin,              /* The create function */
	update_plugin,              /* The update function */
	create_plugin_tab,          /* The config tab create function */
	NULL,                       /* Apply the config function */

	save_plugin_config,         /* Save user config */
	load_plugin_config,         /* Load user config */
	GKFREQ_CONFIG_KEYWORD,      /* config keyword */

	NULL,                       /* Undefined 2 */
	NULL,                       /* Undefined 1 */
	NULL,                       /* private */

	MON_CPU,                    /* Insert plugin before this monitor */
	NULL,                       /* Handle if a plugin, filled in by GKrellM */
	NULL                        /* path if a plugin, filled in by GKrellM */
};


GkrellmMonitor* gkrellm_init_plugin()
{
	style_id = gkrellm_add_meter_style(&plugin_mon, "gkfreq");
	monitor = &plugin_mon;

	return &plugin_mon;
}
