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
            var buffer = new byte[32];
            buffer[0] = (byte)'S';
            buffer[1] = (byte)'E';
            buffer[2] = (byte)'R';
            buffer[3] = (byte)0xaf;
            buffer[4] = 24; // buffer size following header
            buffer[5] = 8;  // data size (we transfer 8 bytes)
            buffer[6] = 0;  //     even though we only use 2 of them
            buffer[7] = 0;  // last 2 bytes of header are 0
			
			int tmp=0;
			
            if (b.A == ButtonState.Pressed)     tmp |= 0x80;
            if (b.X == ButtonState.Pressed)     tmp |= 0x40;
            if (b.Back == ButtonState.Pressed)  tmp |= 0x20;
            if (b.Start == ButtonState.Pressed) tmp |= 0x10;

            if (gpStick)
            {
                var s = gpSt.ThumbSticks;
                if (s.Left.Y > 0.4)  tmp |= 0x08;
                if (s.Left.Y < -0.4) tmp |= 0x04;
                if (s.Left.X < -0.4) tmp |= 0x02;
                if (s.Left.X > 0.4)  tmp |= 0x01;
            }
            else
            {
                var d = gpSt.DPad;
                if (d.IsUp)     tmp |= 0x08;
                if (d.IsDown)   tmp |= 0x04;
                if (d.IsLeft)   tmp |= 0x02;
                if (d.IsRight)  tmp |= 0x01;
            }
            
            buffer[8]=(byte)tmp;
            
            tmp=0;

            if (b.B == ButtonState.Pressed)     tmp |= 0x80;
            if (b.Y == ButtonState.Pressed)     tmp |= 0x40;
            
            if (gpTrigger)
            {
                var t = gpSt.Triggers;
                if (t.Left > 0.4)   tmp |= 0x20;
                if (t.Right > 0.4)  tmp |= 0x10;
            }
            else
            {
                if (b.LeftShoulder == ButtonState.Pressed)  tmp |= 0x20;
                if (b.RightShoulder == ButtonState.Pressed) tmp |= 0x10;
            }
            buffer[9]=(byte)tmp;
            for(tmp=10;tmp<32;tmp++)
				buffer[tmp]=0;

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
