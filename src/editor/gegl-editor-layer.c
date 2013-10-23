/*
 * gegl-editor-layer.c
 *
 *  Created on: 14.08.2012
 *      Author: remy
 */


#include "gegl-editor-layer.h"
#include <stdlib.h>
#include <stdio.h>

void refresh_images(GeglEditorLayer* self)
{
  return;
  GSList*	pair = self->pairs;
  for(;pair != NULL; pair = pair->next)
    {
      node_id_pair	*data = pair->data;

      /*      if(node->image != NULL)
	      cairo_surface_destroy(node->image);	//TODO: only destory if it has changed*/

      const Babl	*cairo_argb32 = babl_format("cairo-ARGB32");

      const GeglRectangle	roi = gegl_node_get_bounding_box(GEGL_NODE(data->node));
      g_print("Rect: %dx%d\n", roi.x, roi.y);

      if(roi.width == 0 || roi.height == 0)
	{
	  g_print("Empty rectangle: %s\n", gegl_node_get_operation(GEGL_NODE(data->node)));
	  continue;		//skip
	}

      gegl_editor_show_node_image(self->editor, data->id);

      gint	stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, roi.width);
      guchar*	buf    = malloc(stride*roi.height);

      //make buffer in memory
      gegl_node_blit(GEGL_NODE(data->node),
		     1.0,
		     &roi,
		     cairo_argb32,
		     buf,
		     GEGL_AUTO_ROWSTRIDE,
		     GEGL_BLIT_CACHE);

      cairo_surface_t*	image =
	cairo_image_surface_create_for_data(buf, CAIRO_FORMAT_ARGB32,
				      roi.width, roi.height,
				      stride);
      //            free(buf);
      gegl_editor_set_node_image(self->editor, data->id, image);
    }
}

gint get_editor_node_id(GeglEditorLayer* self, GeglNode* node)
{
  GSList*		pair = self->pairs;
  for(;pair != NULL; pair = pair->next)
    {
      node_id_pair*	data = pair->data;
      if(data->node == node)
	{
	  return data->id;
	}
    }

  return 0;
}

gint layer_node_removed (gpointer host, GeglEditor* editor, gint node_id)
{
  g_print("remove\n");
  GeglEditorLayer* self = (GeglEditorLayer*)host;
  //TODO: put this in its own function
  GeglNode*		node = NULL;
  GSList*		pair = self->pairs;
  for(;pair != NULL; pair = pair->next)
    {
      node_id_pair*	data = pair->data;
      if(data->id == node_id)
	{
	  node = data->node;
	  break;
	}
    }

  g_assert(node != NULL);

  gegl_node_disconnect_all_pads(node);
  gegl_node_remove_child(self->gegl, node);
}

gint layer_connected_pads (gpointer host, GeglEditor* editor, gint from, const gchar* output, gint to, const gchar* input)
{
  GeglEditorLayer*	self = (GeglEditorLayer*)host;

  GeglNode*	from_node = NULL;
  GeglNode*	to_node	  = NULL;
  GSList*	pair	  = self->pairs;
  for(;pair != NULL; pair = pair->next)
    {
      node_id_pair*	data = pair->data;
      if(data->id == from)
	from_node	     = data->node;
      if(data->id == to)
	to_node		     = data->node;
      if(from_node != NULL && to_node != NULL)
	break;
    }

  g_assert(from_node != NULL && to_node != NULL);
  g_assert(from_node != to_node);
  gboolean	success = gegl_node_connect_to(from_node, output, to_node, input);
  g_print("connected (%d): %s(%s) to %s(%s), %i\n", success, gegl_node_get_operation(from_node), output,
	  gegl_node_get_operation(to_node), input, success);
  refresh_images(self);
}

gint layer_disconnected_pads (gpointer host, GeglEditor* editor, gint from, const gchar* output, gint to, const gchar* input)
{
  GeglEditorLayer*	layer = (GeglEditorLayer*)host;
  g_print("disconnected: %s to %s\n", output, input);
  //TODO: disconnect in GEGL as well
}

struct text_prop_data
{
  GeglNode*		node;
  const gchar*		property;
  GType			prop_type;
  GeglEditorLayer*	layer;
};

void text_property_changed(GtkEntry* entry, gpointer data)
{
  struct text_prop_data *dat  = (struct text_prop_data*)data;
  const gchar			*text = gtk_entry_get_text(entry);

  GeglNode*		 node	   = dat->node;
  const gchar*		 property  = dat->property;
  GType			 prop_type = dat->prop_type;
  GeglEditorLayer	*layer	   = dat->layer;

  g_print("%s -> %s\n", property, text);

  gint	i_value;
  gdouble	 d_value;
  gchar		*str_value;
  GValue	 value = { 0, };	//different than = 0?
  g_value_init(&value, prop_type);
  g_print("%s\n", G_VALUE_TYPE_NAME(&value));

  switch(prop_type)
    {
    case G_TYPE_INT:
      i_value = (gint)strtod(text, NULL);
      gegl_node_set(node, property, i_value, NULL);
      break;
    case G_TYPE_DOUBLE:
      d_value = strtod(text, NULL);
      gegl_node_set(node, property, d_value, NULL);
      break;
    case G_TYPE_STRING:
      gegl_node_set(node, property, text, NULL);
      break;
    default:
      g_print("Unknown property type: %s (%s)\n", property, g_type_name(prop_type));
    }

  refresh_images(layer);
}

typedef struct {
  GeglNode* node;
  const gchar* property;
  GeglEditorLayer* layer;
} select_color_info;

void select_color (GtkButton *widget, gpointer user_data)
{
  GtkColorSelectionDialog* dialog = GTK_COLOR_SELECTION_DIALOG(gtk_color_selection_dialog_new("Select Color")); //todo put the old color selection in

  gint result = gtk_dialog_run(GTK_DIALOG(dialog));

  //

  if(result == GTK_RESPONSE_OK)
    {

      select_color_info* info = (select_color_info*)user_data;
      GtkColorSelection* colsel = GTK_COLOR_SELECTION(dialog->colorsel);

      GdkColor* sel_color = malloc(sizeof(GdkColor));
      gtk_color_selection_get_current_color(colsel, sel_color);

      GeglColor* color = gegl_color_new(NULL);
      gegl_color_set_rgba(color, (double)sel_color->red/65535.0,
			  (double)sel_color->green/65535.0,
			  (double)sel_color->blue/65535.0,
			  (double)gtk_color_selection_get_current_alpha(colsel)/65535.0);

      free(sel_color);

      gegl_node_set(info->node, info->property, color, NULL);

      refresh_images(info->layer);
    }
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

gint layer_node_selected (gpointer host, GeglEditor* editor, gint node_id)
{
  GeglEditorLayer*	self = (GeglEditorLayer*)host;
  GeglNode*		node = NULL;
  GSList*		pair = self->pairs;
  for(;pair != NULL; pair = pair->next)
    {
      node_id_pair*	data = pair->data;
      if(data->id == node_id)
	{
	  node = data->node;
	  break;
	}
    }

  g_assert(node != NULL);

  GeglNode** nodes;
  const gchar** pads;
  gint num = gegl_node_get_consumers(node, "output", &nodes, &pads);

  int i;
  g_print("%s: %d consumer(s)\n", gegl_node_get_operation(node), num);
  for(i = 0; i < num; i++)
    {
      g_print("Connection: (%s to %s)\n", gegl_node_get_operation(node), gegl_node_get_operation(nodes[0]), pads[0]);
    }
  g_print("Input from: %s\n", gegl_node_get_operation(gegl_node_get_producer(node, "input", NULL)));

  //  g_print("selected: %s\n", gegl_node_get_operation(node));

  guint		n_props;
  GParamSpec**	properties = gegl_operation_list_properties(gegl_node_get_operation(node), &n_props);

  //TODO: only create enough columns for the properties which will actually be included (i.e. ignoring GeglBuffer props)
  GtkTable	*prop_table = GTK_TABLE(gtk_table_new(2, n_props, FALSE));

  int d;
  for(d = 0, i = 0; i < n_props; i++, d++)
    {
      GParamSpec*	prop = properties[i];
      GType		type = prop->value_type;
      const gchar*		name = prop->name;

      GtkWidget*	name_label = gtk_label_new(name);
      gtk_misc_set_alignment(GTK_MISC(name_label), 0, 0.5);


      GtkWidget*	value_entry = gtk_entry_new();

      gchar buf[256] = "*";	//can probably be smaller; In fact, can probably do this without sprintf and a buffer. TODO: look at g_string

      gint	i_value;
      gdouble	d_value;
      gchar*	str_value;
      gboolean skip = FALSE;

      switch(type)
	{
	case G_TYPE_INT:
	  gegl_node_get(node, name, &i_value, NULL);
	  sprintf(buf, "%d", i_value);
	  break;
	case G_TYPE_DOUBLE:
	  gegl_node_get(node, name, &d_value, NULL);
	  sprintf(buf, "%.3f", d_value);
	  break;
	case G_TYPE_STRING:
	  gegl_node_get(node, name, &str_value, NULL);
	  sprintf(buf, "%s", str_value);
	  break;
	}

      if(type == GEGL_TYPE_BUFFER) {
	skip = TRUE;
	d--;
      } else if( type == GEGL_TYPE_COLOR) {
	skip = TRUE;
	GtkWidget *color_button = gtk_button_new_with_label("Select");

	select_color_info* info = malloc(sizeof(select_color_info));
	info->node = node;
	info->property = name;
	info->layer = self;

	g_signal_connect(color_button, "clicked", (GCallback)select_color, info);

	gtk_table_attach(prop_table, name_label, 0, 1, d, d+1, GTK_FILL, GTK_FILL, 1, 1);
	gtk_table_attach(prop_table, color_button, 1, 2, d, d+1, GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 1, 1);
      }

      if(!skip)
	{
	  gtk_entry_set_text(GTK_ENTRY(value_entry), buf);

	  gtk_entry_set_width_chars(GTK_ENTRY(value_entry), 2);
	  struct text_prop_data	*data = malloc(sizeof(struct text_prop_data));	//TODO store this in a list and free it when the node is deselected
	  data->node		      = node;
	  data->property		      = name;
	  data->prop_type		      = type;
	  data->layer		      = self;
	  g_signal_connect(value_entry, "activate", G_CALLBACK(text_property_changed), data);

	  gtk_table_attach(prop_table, name_label, 0, 1, d, d+1, GTK_FILL, GTK_FILL, 1, 1);
	  gtk_table_attach(prop_table, value_entry, 1, 2, d, d+1, GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 1, 1);
	}
    }

  //  gegl_node_process(node);
  GeglGtkView *gtk_view = gegl_gtk_view_new_for_node(node);

  GeglRectangle rect = gegl_node_get_bounding_box(node);

  if(gegl_rectangle_is_infinite_plane(&rect))
    {
      gegl_gtk_view_set_autoscale_policy(gtk_view, GEGL_GTK_VIEW_AUTOSCALE_DISABLED);
      gegl_gtk_view_set_scale(gtk_view, 1.0);
      g_print("Disable autoscale: scale=%f, x=%f, y=%f\n", gegl_gtk_view_get_scale(gtk_view),
      gegl_gtk_view_get_x(gtk_view), gegl_gtk_view_get_y(gtk_view));
    }

  gtk_widget_show(GTK_WIDGET(gtk_view));

  //TODO: draw checkerboard under preview to indicate transparency

  gtk_box_pack_start(GTK_BOX(self->prop_box), GTK_WIDGET(prop_table), FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(self->prop_box), GTK_WIDGET(gtk_view), TRUE, TRUE, 10);

  GtkWidget* label = gtk_label_new("Click the image\nto open in a\nnew window");
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start(GTK_BOX(self->prop_box), label, FALSE, TRUE, 10);

  gtk_widget_show_all(self->prop_box);
}

gint layer_node_deselected(gpointer host, GeglEditor* editor, gint node)
{
  GeglEditorLayer*	 self = (GeglEditorLayer*)host;
  GList			*children, *iter;

  children = gtk_container_get_children(GTK_CONTAINER(self->prop_box));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  g_list_free(children);
}

/*	gint (*nodeSelected) (gpointer host, GeglEditor* editor, gint node);
    gint (*nodeDeselected) (gpointer host, GeglEditor* editor, gint node);*/

GeglEditorLayer*
layer_create(GeglEditor* editor, GeglNode* gegl, GtkWidget* prop_box)
{
  GeglEditorLayer*	layer = malloc(sizeof(GeglEditorLayer));
  editor->host		      = (gpointer)layer;
  editor->connectedPads	      = layer_connected_pads;
  editor->disconnectedPads    = layer_disconnected_pads;
  editor->nodeSelected	      = layer_node_selected;
  editor->nodeDeselected      = layer_node_deselected;
  editor->nodeRemoved      = layer_node_removed;
  layer->editor		      = editor;
  layer->gegl		      = gegl;
  layer->pairs		      = NULL;
  layer->prop_box	      = prop_box;
  return layer;
}

gpointer gegl_node_get_pad(GeglNode* self, const gchar* name);
const gchar*	gegl_pad_get_name(gpointer pad);
GSList*	gegl_node_get_pads(GeglNode *self);
GSList*	gegl_node_get_input_pads(GeglNode *self);
gpointer gegl_node_get_pad (GeglNode      *self, const gchar   *name);

static void print_info(GeglNode* gegl)
{
  GSList *list = gegl_node_get_children(gegl);
  for(;list != NULL; list = list->next)
    {
      GeglNode* node = GEGL_NODE(list->data);
      g_print("Node %s\n", gegl_node_get_operation(node));

      if(gegl_node_get_pad(node, "output") == NULL) {
	g_print("Output pad is NULL\n");
      }

      /*      GeglNode** nodes;
      const gchar** pads;
      gint num = gegl_node_get_consumers(node, "output", &nodes, &pads);
      g_print("%s: %d consumer(s)\n", gegl_node_get_operation(node), num);

      int i;
      for(i = 0; i < num; i++)
	{
	  g_print("Connection: (%s to %s)\n", gegl_node_get_operation(node), gegl_node_get_operation(nodes[0]), pads[0]);
	}
	g_print("\n");*/
    }
}

void layer_set_graph(GeglEditorLayer* self, GeglNode* gegl)
{
  //properly dispose of old gegl graph
  self->gegl = gegl;
  gegl_editor_remove_all_nodes(self->editor);

  GSList *list = gegl_node_get_children(gegl);
  for(;list != NULL; list = list->next)
    {
      GeglNode* node = GEGL_NODE(list->data);
      g_print("Loading %s\n", gegl_node_get_operation(node));
      layer_add_gegl_node(self, node);
    }

  list = gegl_node_get_children(gegl);

  for(list = g_slist_reverse(list); list != NULL; list = list->next)
    {
      GeglNode* node = GEGL_NODE(list->data);
      gint from = get_editor_node_id(self, node);

      GeglNode** nodes;
      const gchar** pads;

      if(!gegl_node_has_pad(node, "output")) {
    	  break;}

      gint num = gegl_node_get_consumers(node, "output", &nodes, &pads);

      int i;
      g_print("%s: %d consumer(s)\n", gegl_node_get_operation(node), num);
      for(i = 0; i < num; i++)
	{
	  gint to = get_editor_node_id(self, nodes[i]);
	  g_print("Connecting to consumer (%s to %s): output->%s\n", gegl_node_get_operation(node), gegl_node_get_operation(nodes[0]), pads[0]);
	  gegl_editor_add_connection(self->editor, from, to, "output", pads[0]);
	}
    }
}

void
layer_add_gegl_node(GeglEditorLayer* layer, GeglNode* node)
{
  //get input pads
  //gegl_pad_is_output
  GSList	*pads	    = gegl_node_get_input_pads(node);
  guint		 num_inputs = g_slist_length(pads);
  gchar**	 inputs	    = malloc(sizeof(gchar*)*num_inputs);
  int		 i;
  for(i = 0; pads != NULL; pads = pads->next, i++)
    {
      inputs[i] = (gchar*)gegl_pad_get_name(pads->data);
    }

  gint	id;
  if(gegl_node_get_pad(node, "output") == NULL)
    {
      id = gegl_editor_add_node(layer->editor, gegl_node_get_operation(node), num_inputs, inputs, 0, NULL);
    }
  else
    {
      gchar*	output = "output";
      gchar* outputs[] = {output};
      id	       = gegl_editor_add_node(layer->editor, gegl_node_get_operation(node), num_inputs, inputs, 1, outputs);
    }

  node_id_pair* new_pair = malloc(sizeof(node_id_pair));
  new_pair->node	 = node;
  new_pair->id		 = id;
  layer->pairs		 = g_slist_append(layer->pairs, new_pair);
}


void
gegl_node_disconnect_all_pads(GeglNode* node)
{
  GSList* list;
  for(list = gegl_node_get_pads(node); list != NULL; list = list->next)
    {
      if(gegl_pad_is_input(list->data)) //disconnect inputs
	{
	  gegl_node_disconnect(node, (gchar*)gegl_pad_get_name(list->data));
	}
      else if(gegl_pad_is_output(list->data)) //disconnect outputs
	{
	  GeglNode** nodes;
	  const gchar** pads;
	  gint num_consumers = gegl_node_get_consumers(node, gegl_pad_get_name(list->data), &nodes, &pads);
	  gint i;
	  for(i = 0; i < num_consumers; i++)
	    {
	      gegl_node_disconnect(nodes[i], pads[i]);
	    }
	}
    }
}
