using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Origin : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        
    }
    private void Awake()
    {

    }

    // Update is called once per frame
    void Update()
    {
        Debug.Log("I am the Origin at " + this.transform.position);
        PlayerHandler.originPosition = this.transform.position;
        PlayerHandler.originRotation = this.transform.rotation;
    }
}
