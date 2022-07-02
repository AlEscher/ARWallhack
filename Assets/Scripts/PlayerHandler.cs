using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerHandler : MonoBehaviour
{
    [SerializeField] GameObject playerPrefab;
    Dictionary<int, GameObject> knownPlayers;
    public static Vector3 originPosition;
    NetworkCoordinateClient net;

    // Start is called before the first frame update
    void Start()
    {
        this.net = this.gameObject.GetComponent<NetworkCoordinateClient>();
        this.knownPlayers = new Dictionary<int, GameObject>();
    }

    // Update is called once per frame
    void Update()
    {
        Debug.Log("Player Handler knows the Origin is sitting at " + originPosition);
        Debug.Log("Player handler knows " + knownPlayers.Count + " Players");
        //if (originPosition == Vector3.zero) return;
        Dictionary<int, Vector3> currentPlayers = this.net.GetTargets();
        while(knownPlayers.Values.Count > currentPlayers.Values.Count)
        {
            knownPlayers.Remove(knownPlayers.Values.Count - 1);
        }
        while(knownPlayers.Values.Count < currentPlayers.Values.Count)
        {
            knownPlayers.Add(knownPlayers.Values.Count, Instantiate(playerPrefab, Vector3.zero, Quaternion.identity));
        }
        for(int i = 0; i < knownPlayers.Count; i++)
        {
            knownPlayers[i].transform.position = currentPlayers[i] + originPosition;
        }        
    }
}
