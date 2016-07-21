﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Runtime.InteropServices;
using System.Threading;

namespace testApi
{
    class Program
    {
        static void Main(string[] args)
        {
            string rtmp_push_url = "rtmp://2453.livepush.myqcloud.com/live/2453_993e47c3f64311e5b91fa4dcbef5e35a?bizid=2453";
            string parameter = "-g 640x480 -b 3000 -f 25/1 -gop 25";
            string record_file = "E:\\1.MP4";

            int time_span = 1*15;
            CFunction.encoder_start(parameter, record_file);
            //string image_file = "E:\\workspace\\encoder\\HappyFish.jpg";
            //string image_file = "E:\\workspace\\encoder\\HappyFish.png";
           // CFunction.encoder_add_image0(image_file,100,100, 0.5);
            //CFunction.encoder_set_image_position(1100,100,0,0 );

            int i = 0;
             while (true) {
             //CFunction.encoder_set_fontcolor(0, 188, 177, 188);
             //CFunction.encoder_overlap("爱奇艺", 1100, 80, 10);
                Thread.Sleep((time_span + 1) * 1000);
                string format = "e:\\svn20160118\\BilliardTrain\\bin\\Debug\\0001-1-1 0#00#00_0_0_0_0_0_0_0_0_00000000-0000-0000-0000-000000000000_{0}.mp4";
                string file_name = string.Format(format, ++i);
                int ret = CFunction.encoder_output(file_name,time_span-10);
                System.Console.Write("ret:{0}", ret);

                if (i == 3)
                    break;
            }


            System.Console.Write("call encoder_stop after 6 seconds!! ");
            Thread.Sleep((5 + 1) * 1000);

            CFunction.encoder_stop();


        }
    }


    public class CFunction
    {

        //TODO FIXME:

        [DllImport("myEncoder.dll", EntryPoint = "encoder_rtmp_push", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public extern static int encoder_rtmp_push(string addr);

        [DllImport("myEncoder.dll", EntryPoint = "encoder_start", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public extern static int encoder_start(string parameter, string record_name);

        [DllImport("myEncoder.dll", EntryPoint = "encoder_stop", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public extern static int encoder_stop();

        [DllImport("myEncoder.dll", EntryPoint = "encoder_output", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int encoder_output(string file_name, int time_span);

    }

}