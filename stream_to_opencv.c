#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <gst/gst.h>
#include <glib.h>
#include <stdlib.h>

static gboolean bus_call(GstBus *bus,GstMessage *msg,gpointer data){
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;

}

int main(int argc, char *argv[]) {
    int pipe_fds[2];
    pid_t pid;

    pipe(pipe_fds);
//run raspivid as a child process to capture raspberry pi video capture and send to the write end of the pipe
    //char *cmd[] = {"raspivid", "-t 0 -w 1280 -h 720 -fps 60 -hf -b 2000000 -o -", 0};

    if ((pid = fork()) == 0) { /* child */
        dup2(pipe_fds[1], 1); // set stdout of the process to the write end of the pipe
        close(pipe_fds[0]);

        GMainLoop *loop;
        GstElement *pipe,*src,*rtp,*sink,*avdec,*convert ;
        GstElementFactory *factory;
        GstBus *bus;
        GstCaps *srcCaps;
        guint bus_watch_id;

        gst_init(&argc,&argv);
        loop=g_main_loop_new(NULL,FALSE);


        pipe=gst_pipeline_new(NULL);
        src=gst_element_factory_make("udpsrc",NULL);      // fdsrc: default is stdin, is it right?
        sink=gst_element_factory_make("fdsink",NULL);
        avdec=gst_element_factory_make("avdec_h264",NULL);
        convert=gst_element_factory_make("videoconvert",NULL);


        srcCaps=gst_caps_new_simple("application/x-rtp",
          "media",G_TYPE_STRING, "audio",
          "clock-rate", G_TYPE_INT,90000,
          "encoding-name", G_TYPE_STRING,"H264",
          NULL);

        g_object_set(sink,"sync",FALSE,NULL);
        g_object_set(src,"port",5000,NULL);
        g_object_set(src,"caps",srcCaps,NULL);
        gst_caps_unref(srcCaps);

        rtp=gst_element_factory_make("rtph264depay",NULL);

        

      /*check if elements init successfully*/
        if(!(pipe&&src&&sink&&avdec&&rtp&&convert)){
          g_print("Fail to init factories!\n");
          return -1;
        }

      /*set up the pipeline*/
        gst_bin_add_many(GST_BIN(pipe),src,rtp,avdec,convert,sink,NULL);
        //gst_element_link_filtered(src,parse,srcCaps);
        //gst_caps_unref(srcCaps);

        gst_element_link_many(src,rtp,avdec,convert,sink,NULL);
        bus=gst_pipeline_get_bus(GST_PIPELINE(pipe));
        bus_watch_id=gst_bus_add_watch(bus,bus_call,loop);  
        gst_object_unref(bus);


      /*set the pipeline to playing state*/
        g_print("Playing");
        gst_element_set_state(pipe,GST_STATE_PLAYING);
        g_main_loop_run(loop);

      /*clean up*/
        g_print("Clean up");
        gst_object_unref(GST_OBJECT(pipe));
        g_main_loop_unref(loop);

              //execvp(cmd[0], cmd); // execute the program.
              //fflush(stdout);
            
              exit(0);
          } else if (pid == -1) { /* failed */
              perror("fork");
              exit(1);
          } 

        /* parent  to read from the input end of the pipe*/
          dup2(pipe_fds[0], 0);
          close(pipe_fds[1]);
          
          char *cmd[] = {"./Test",  0};//the OpenCV program we want to run
          execvp(cmd[0], cmd); // execute the program.
          perror(cmd[0]);


  return 0;
}
