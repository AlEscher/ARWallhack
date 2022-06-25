using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class OtherPlayerBehaviour : MonoBehaviour
{

    private Vector3 getPosition()
    {
        //interface with transmission here
        return Vector3.left * (Time.realtimeSinceStartup % 3) + this.transform.parent.position;
    }

    // Start is called before the first frame update
    void Start()
    {
        this.transform.position = Vector3.zero + this.transform.parent.position;
    }

    // Update is called once per frame
    void Update()
    {
        this.transform.position = this.getPosition();
        Debug.Log(this.getPosition());
    }
}
