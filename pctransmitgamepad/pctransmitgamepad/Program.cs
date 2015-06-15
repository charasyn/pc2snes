using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;
using OpenTK.Input;

namespace pctransmitgamepad
{
    class Program
    {
        static bool gpStick = false, gpTrigger = false;
        static byte[] MakeBufferFromGPState(GamePadState gpSt)
        {
            var b = gpSt.Buttons;
            var buffer = new byte[4];
            buffer[0] = (byte)'r';
            buffer[1] = 0x10; // length
            buffer[2] = 0;
            buffer[3] = 0;

            if (b.A == ButtonState.Pressed)     buffer[2] |= 0x01;
            if (b.X == ButtonState.Pressed)     buffer[2] |= 0x02;
            if (b.Start == ButtonState.Pressed) buffer[2] |= 0x04;
            if (b.Back == ButtonState.Pressed)  buffer[2] |= 0x08;

            if (b.B == ButtonState.Pressed)     buffer[3] |= 0x01;
            if (b.Y == ButtonState.Pressed)     buffer[3] |= 0x02;

            if (gpStick)
            {
                var s = gpSt.ThumbSticks;
                if (s.Left.Y > 0.4)  buffer[2] |= 0x10;
                if (s.Left.Y < -0.4) buffer[2] |= 0x20;
                if (s.Left.X < -0.4) buffer[2] |= 0x40;
                if (s.Left.X > 0.4)  buffer[2] |= 0x80;
            }
            else
            {
                var d = gpSt.DPad;
                if (d.IsUp)     buffer[2] |= 0x10;
                if (d.IsDown)   buffer[2] |= 0x20;
                if (d.IsLeft)   buffer[2] |= 0x40;
                if (d.IsRight)  buffer[2] |= 0x80;
            }

            if (gpTrigger)
            {
                var t = gpSt.Triggers;
                if (t.Left > 0.4)   buffer[3] |= 0x4;
                if (t.Right > 0.4)  buffer[3] |= 0x8;
            }
            else
            {
                if (b.LeftShoulder == ButtonState.Pressed)  buffer[3] |= 0x4;
                if (b.RightShoulder == ButtonState.Pressed) buffer[3] |= 0x8;
            }

            return buffer;
        }
        static void Main(string[] args)
        {
            int gpIndex=-1;
            for (int i = 0; gpIndex==-1 && i < 4; i++)
            {
                var cap = GamePad.GetCapabilities(i);
                if(!cap.IsConnected)
                    continue;
                if (!(cap.HasAButton && cap.HasBButton && cap.HasXButton && cap.HasYButton &&
                    cap.HasStartButton && cap.HasBackButton))
                    continue;
                if (cap.HasDPadDownButton && cap.HasDPadLeftButton &&
                    cap.HasDPadRightButton && cap.HasDPadUpButton)
                    gpStick = false;
                else
                    if (cap.HasLeftXThumbStick && cap.HasLeftYThumbStick)
                        gpStick = true;
                    else
                        continue;
                if (cap.HasLeftShoulderButton && cap.HasRightShoulderButton)
                    gpTrigger = false;
                else
                    if (cap.HasLeftTrigger && cap.HasRightTrigger)
                        gpTrigger = true;
                    else
                        continue;
                gpIndex = i;
            }
            if(gpIndex<0||gpIndex>=4)
            {
                Console.WriteLine("No valid gamepad found. Exiting.");
                return;
            }
            using (var sp = new SerialPort("COM3", 115200))
            {
                sp.DataBits = 8;
                sp.StopBits = StopBits.One;
                sp.Parity = Parity.None;
                sp.Handshake = Handshake.None;

                sp.Open();
                bool go = true;
                while (go)
                {
                    var ti = new System.Diagnostics.Stopwatch();
                    ti.Start();
                    var c = (char)sp.ReadChar();
                    ti.Stop();
                    var bufStr = string.Format("{0:F2}",ti.Elapsed.TotalMilliseconds) + " ";
                    if(c=='N')
                    {
                        ti.Restart();
                        var buffer = MakeBufferFromGPState(GamePad.GetState(gpIndex));
                        sp.Write(buffer, 0, buffer.Length);
                        sp.DiscardInBuffer();
                        ti.Stop();
                        bufStr += string.Format("{0:F2}", ti.Elapsed.TotalMilliseconds) + " ";
                        for (int i = 0; i < buffer.Length; i++)
                        {
                            bufStr += string.Format("{0:X}", buffer[i])+' ';
                        }
                        Console.Clear();
                        Console.WriteLine(bufStr);
                    }
                }
            }
        }
    }
}
