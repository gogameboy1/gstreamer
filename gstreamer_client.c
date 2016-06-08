#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <gst/gst.h>
#include <glib.h>
#include <stdlib.h>

static gboolean bus_call(GstBus *bus , GstMessage *msg , gpointer data)
{
  GMainLoop *loop = (GMainLoop *)data;

  switch (GST_MESSAGE_TYPE (msg))
  {
    case GST_MESSAGE_EOS:
      g_print("End of stream\n");
      g_main_loop_quit(loop);
      break;

      case GST_MESSAGE_ERROR:
      {
        gchar *debug;
        GError *error;

        gst_message_parse_error(msg, &error , &debug);
        g_free(debug);

        g_printerr("Error: %s\n" , error->message);
        g_error_free(error);

        g_main_loop_quit(loop);
        break;
      }
      default:
        break;
  }
  return TRUE;
}

int main(int argc , char *argv[])
{
  int pipe_fds[2];
  pid_t pid;

  pipe(pipe_fds);

  char *cmd[] = {"/opt/vc/bin/raspivid" , "-t 0 -w 1280 -h 720 -fps 60 -hf -b 2000000 -o -" , 0};

  if((pid = fork()) == 0)
  {
    dup2(pipe_fds[1] , 1);
    close(pipe_fds[0]);
    execvp(cmd[0] , cmd);

    perror(cmd[0]);
    exit(0);
  
  }else if(pid == -1)
  {
    perror("fork");
    exit(1);
  }

  dup2(pipe_fds[0] , 0);
  close(pipe_fds[1]);

    GMainLoop *loop;
    GstElement *pipe , *src , *parse , *rtp , *sink;
    GstElementFactory *factory;
    GstBus *bus;
    GstCaps *srcCaps;
    guint bus_watch_id;

    gst_init(&argc , &argv);
    loop = g_main_loop_new(NULL , FALSE);
    

    pipe = gst_pipeline_new(NULL);
    src = gst_element_factory_make("fdsrc" , NULL);
    sink = gst_element_factory_make("udpsink" , NULL);

    g_object_set(sink , "host" , "192.168.1.187" , NULL);
    g_object_set(sink , "port" , 5000 , NULL);

    parse = gst_element_factory_make("h264parse" , NULL);

    rtp = gst_element_factory_make("rtph264pay" , NULL);





    if(!(pipe && src && sink && parse && rtp))
    {
      g_print("Fail to init factories!\n");
      return -1;
    }


    gst_bin_add_many(GST_BIN(pipe) , src , parse , rtp , sink , NULL);


    gst_element_link_many(src , parse , rtp , sink , NULL);
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    bus_watch_id = gst_bus_add_watch(bus , bus_call , loop);
    gst_object_unref(bus);


    g_print("Playing");
    gst_element_set_state(pipe , GST_STATE_PLAYING);
    g_main_loop_run(loop);

    
    g_print("Clean up");
    gst_object_unref(GST_OBJECT(pipe));
    g_main_loop_unref(loop);

    return 0;

}
