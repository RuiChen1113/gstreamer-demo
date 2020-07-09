#include <gst/gst.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *tee;
  GstElement *queue;
  GstElement *mux;
  GstElement *filesink;
	GstPad *video_pad;
} CustomData;

CustomData data;
static bool need_release = false;

void release();

void sig_handler(int signo)
{
	static int flag = 0;

	if (signo == SIGINT)
	{
		printf("received SIGINT1\n");
		if(flag == 0)
		{
			record_start();
			flag = 1;
		}else{
			flag = 0;
			record_stop();
		}
	}

}

void record_start()
{
	printf("%s start\n", __func__);
	if(need_release){
		release();
		usleep(10);
		need_release = false;
	}


	char filename[32] = {0};
	static num = 0;
	sprintf(filename, "test_num_%d.mkv", ++num);

	data.tee = gst_bin_get_by_name(GST_BIN (data.pipeline), "tee_jpegenc");
	data.queue = gst_element_factory_make ("queue", NULL);
	data.mux = gst_element_factory_make ("matroskamux", NULL);
	data.filesink = gst_element_factory_make ("filesink", NULL);
	gst_util_set_object_arg ((GObject *)data.filesink, "location", filename);


	gst_object_ref(data.queue);
	gst_object_ref(data.mux);
	gst_object_ref(data.filesink);

	gst_bin_add_many (GST_BIN (data.pipeline), data.queue, data.mux, data.filesink, NULL);

	if (!gst_element_link_many  (data.queue, data.mux, data.filesink, NULL)) {
		g_printerr ("Elements could not be linked.\n");
		gst_object_unref (data.pipeline);
		return -1;
	}
	data.video_pad = gst_element_get_request_pad(data.tee, "src_%u");
	GstPad *queue_mux_pad = gst_element_get_static_pad (data.queue, "sink");

	gst_element_sync_state_with_parent(data.queue);
	gst_element_sync_state_with_parent(data.mux);
	gst_element_sync_state_with_parent(data.filesink);

	gst_pad_link (data.video_pad, queue_mux_pad);
	gst_object_unref(queue_mux_pad);

	printf("%s stop\n", __func__);
}

void release()
{
	printf("%s start\n", __func__);
	gst_bin_remove (GST_BIN (data.pipeline), data.queue);
	gst_bin_remove (GST_BIN (data.pipeline), data.mux);
	gst_bin_remove (GST_BIN (data.pipeline), data.filesink);

	gst_element_set_state (data.queue, GST_STATE_NULL);
	gst_element_set_state (data.mux, GST_STATE_NULL);
	gst_element_set_state (data.filesink, GST_STATE_NULL);

	gst_object_unref(data.queue);
	gst_object_unref(data.mux);
	gst_object_unref(data.filesink);

	gst_element_release_request_pad (data.tee, data.video_pad);
	gst_object_unref(data.video_pad);
	printf("%s stop\n", __func__);
}

void record_stop()
{
	printf("%s start\n", __func__);
	GstPad *queue_mux_pad = gst_element_get_static_pad (data.queue, "sink");
	int num = 0;
	gst_pad_unlink (data.video_pad, queue_mux_pad);
	gst_pad_send_event (queue_mux_pad, gst_event_new_eos());
	gst_object_unref(queue_mux_pad);	
	need_release = true;
	printf("%s stop\n", __func__);
}


int main(int argc, char *argv[]) {

  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;
  if (signal(SIGINT, sig_handler) == SIG_ERR)
 	{
 		exit(-1);
 	}
  /* Initialize GStreamer */
  gst_init (&argc, &argv);
  //gst-launch-1.0 videotestsrc ! tee name=tee1  tee1. ! queue ! matroskamux ! filesink location=111.mkv
  /* Create the elements */


  /* Create the empty pipeline */
  data.pipeline = gst_parse_launch("gst-launch-1.0 -e videotestsrc ! jpegenc ! tee name=tee_jpegenc tee_jpegenc. ! queue ! fakesink", NULL);
	
  ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }
  sleep(1);
    GstState gst_state;
    gst_element_get_state (data.pipeline, &gst_state, NULL, 1);
	printf("pipe state = %d\n", gst_state);

//record_start();

  /* Connect to the pad-added signal */
  //g_signal_connect (data.tee, "pad-added", G_CALLBACK (pad_added_handler), &data);


  printf("Listen to the bus\n");
  /* Listen to the bus */
  bus = gst_element_get_bus (data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Parse message */
    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error (msg, &err, &debug_info);
          g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
          g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
          g_clear_error (&err);
          g_free (debug_info);
          terminate = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print ("End-Of-Stream reached.\n");
          //terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          /* We are only interested in state-changed messages from the pipeline */
          if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
            g_print ("Pipeline state changed from %s to %s:\n",
                gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
          }
          break;
        default:
          /* We should not reach here */
          g_printerr ("Unexpected message received.\n");
          break;
      }
      gst_message_unref (msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (data.pipeline, GST_STATE_NULL);
  gst_object_unref (data.pipeline);
  return 0;
}

/* This function will be called by the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) {
  GstPad *sink_pad = gst_element_get_static_pad (data->queue, "sink");
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

  /* If our converter is already linked, we have nothing to do here */
  if (gst_pad_is_linked (sink_pad)) {
    g_print ("We are already linked. Ignoring.\n");
    goto exit;
  }

  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);
  if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
    g_print ("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
    goto exit;
  }

  /* Attempt the link */
  ret = gst_pad_link (new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("Type is '%s' but link failed.\n", new_pad_type);
  } else {
    g_print ("Link succeeded (type '%s').\n", new_pad_type);
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);

  /* Unreference the sink pad */
  gst_object_unref (sink_pad);
}
