using System;
using System.Net.Sockets;
using System.Net;
using UnityEngine;
using System.Collections.Generic;

public class NetworkCoordinateClient : MonoBehaviour
{
    [SerializeField]
    string m_ipAddress;
    [SerializeField]
    int m_Port;

    private Socket m_Socket;

    // Start is called before the first frame update
    void Start()
    {
        try
        {
            // Parsing server IPAddress and creating server endpoint
            IPAddress ipAddr = IPAddress.Parse(m_ipAddress);
            IPEndPoint serverEndPoint = new IPEndPoint(ipAddr, m_Port);

            // Creation TCP/IP Socket using
            // Socket Class Constructor
            m_Socket = new Socket(ipAddr.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

            // Connect Socket to the remote endpoint
            m_Socket.Connect(serverEndPoint);
            Debug.LogFormat("Socket connected to {0} ", m_Socket.RemoteEndPoint.ToString());
        }
        // Manage socket creation exceptions
        catch (Exception e)
        {
            Debug.LogErrorFormat("Failed to establish connection to {0}:{1} with error: {2} ", m_ipAddress, m_Port, e.Message);
        }
    }

    // Update is called once per frame
    void Update()
    {
        try
        {
            byte[] messageBuffer = new byte[2048];
            // Receive the message. This returns number of bytes received
            int byteRecv = m_Socket.Receive(messageBuffer);
            Debug.LogFormat("Received: {0}", byteRecv);
        }

        catch (Exception e)
        {
            Debug.LogErrorFormat("Failed to receive data: {0}", e.ToString());
        }
    }

    public Dictionary<int, Vector3> GetTargets()
    {
        return null;
    }
}
