using System;
using System.Net.Sockets;
using System.Net;
using UnityEngine;
using System.Collections.Generic;
using System.Text;
using System.Threading;

public class NetworkCoordinateClient : MonoBehaviour
{
    [Serializable]
    private class NetworkCoordinate
    {
        public int Id { get; }
        public Vector3 Position { get; }
    }

    [SerializeField]
    [Tooltip("The IP address of the server that the client should connect to, you can run ipconfig in a terminal to find this.")]
    string m_ipAddress;
    [SerializeField]
    [Tooltip("The port of the server that the client should connect to.")]
    int m_Port = 20042;

    private Socket m_Socket;
    // Have to track connection separately since Socket::Connected does not work
    private bool m_bConnected = false;
    private Dictionary<int, Vector3> m_Targets;
    private ReaderWriterLockSlim m_DictLock;

    // Start is called before the first frame update
    void Start()
    {
        m_Targets = new Dictionary<int, Vector3>();
        m_DictLock = new ReaderWriterLockSlim();
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
            LogInfo("Socket connected to {0} ", m_Socket.RemoteEndPoint.ToString());
            m_bConnected = true;
        }
        // Manage socket creation exceptions
        catch (Exception e)
        {
            LogError("Failed to establish connection to {0}:{1} with error: {2} ", m_ipAddress, m_Port, e.Message);
        }
    }

    // Update is called once per frame
    void Update()
    { 
        try
        {
            if (!m_bConnected) return;

            byte[] buffer = new byte[2048];
            // Receive the message. This returns number of bytes received
            int bytesReceived = m_Socket.Receive(buffer);
            if (bytesReceived == 0)
            {
                m_bConnected = false;
                LogWarning("Connection with {0}:{1} was closed.", m_ipAddress, m_Port);
                return;
            }

            LogInfo("Received {0} bytes from {1}", bytesReceived, m_Socket.RemoteEndPoint.ToString());
            ReadCoordinatesFromBuffer(buffer, bytesReceived);
        }
        catch (Exception e)
        {
            LogError("Failed to receive data: {0}", e.ToString());
        }
        finally
        {
            if (m_DictLock.IsWriteLockHeld)
            {
                m_DictLock.ExitWriteLock();
            }
        }
    }

    void OnDestroy()
    {
        m_DictLock.Dispose();
        try
        {
            m_Socket.Shutdown(SocketShutdown.Both);
        }
        finally
        {
            m_Socket.Close();
        }
    }

    public Dictionary<int, Vector3> GetTargets()
    {
        m_DictLock.EnterReadLock();
        try
        {
            return m_Targets;
        }
        finally
        {
            m_DictLock.ExitReadLock();
        }
    }

    public Vector3 GetTarget(int idx)
    {
        m_DictLock.EnterReadLock();
        try
        {
            return m_Targets[idx];
        }
        finally
        {
            m_DictLock.ExitReadLock();
        }
    }

    private void ReadCoordinatesFromBuffer(byte[] buffer, int bytesReceived)
    {
        string[] jsonStrings = Encoding.ASCII.GetString(buffer, 0, bytesReceived).Split('\n');
        m_DictLock.EnterWriteLock();
        foreach (string jsonElement in jsonStrings)
        {
            NetworkCoordinate[] coordinates = JsonUtility.FromJson<NetworkCoordinate[]>(jsonElement);
            foreach (NetworkCoordinate coordinate in coordinates)
            {
                m_Targets[coordinate.Id] = coordinate.Position;
            }
        }
    }

    // Prepends this script's name to the log output to facilitate filtering
    private void LogInfo(string format, params object[] args)
    {
        Debug.LogFormat("{0}: {1}", GetType().ToString(), string.Format(format, args));
    }

    // Prepends this script's name to the log output to facilitate filtering
    private void LogError(string format, params object[] args)
    {
        Debug.LogErrorFormat("{0}: {1}", GetType().ToString(), string.Format(format, args));
    }

    private void LogWarning(string format, params object[] args)
    {
        Debug.LogWarningFormat("{0}: {1}", GetType().ToString(), string.Format(format, args));
    }
}
