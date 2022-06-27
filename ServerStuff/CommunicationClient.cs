using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace CommunicationClient
{
    class Program
    {
        static void Main(string[] args)
        {

            if(args.Length == 0)
            {
                Console.WriteLine("Please specify an Server IP-Address as the first argument");
                return;
            }

            Socket sock = setupSocket(args[0]);
            byte[] messageBuffer = new byte[2048];

            for (int i =0; i<6; i++)
            {
                int byteRecv = receiveData(ref sock, ref messageBuffer);
                Console.WriteLine("Message from Server {0}", Encoding.ASCII.GetString(messageBuffer, 0, byteRecv));
            }

        }

        static Socket setupSocket(String ServerAddress)
        {
            try
            {
                // Parsing server IPAddress and creating server endpoint
                IPAddress ipAddr = IPAddress.Parse(ServerAddress);
                IPEndPoint serverEndPoint = new IPEndPoint(ipAddr, 20042);

                // Creation TCP/IP Socket using
                // Socket Class Constructor
                Socket sock = new Socket(ipAddr.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

                // Connect Socket to the remote endpoint
                sock.Connect(serverEndPoint);
                Console.WriteLine("Socket connected to {0} ", sock.RemoteEndPoint.ToString());

                return sock;
            }
            // Manage socket creation exceptions
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
                return null;
            }

        }

        static int receiveData(ref Socket sock, ref byte[] messageBuffer)
        {
            try
            {
                // Receive the message. This returns number of bytes received
                int byteRecv = sock.Receive(messageBuffer);
                return byteRecv;
            }

            catch (Exception e)
            {
                Console.WriteLine("Unexpected exception : {0}", e.ToString());
                return 0;
            }
        }


    }


}
