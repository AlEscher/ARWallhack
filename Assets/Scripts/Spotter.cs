using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Spotter : MonoBehaviour
{
    //global camera variables
    [SerializeField] Vector3 cameraRotation;
    [SerializeField] float focalLength; //in millimeter
    [SerializeField] float cameraHeight; //in millimeter, position of the camra over the ground
    [SerializeField] int imageWidth; //in pixel
    [SerializeField] int imageHeight; //in pixel
    //global marker height (i.e. height of the tracked person)
    [SerializeField] float trackerHeight; //in millimeter
    //resulting position of the head of the tracked person
    public Vector3 head;
    //resulting position of the foot of the tracked person
    public Vector3 foot;

    //interface to marker tracking, get current position of marker in image
    private Vector2 GetImagePoint()
    {
        //TODO: get marker tracking, return image coordinates through this method
        return Vector2.zero;
    }


    // Start is called before the first frame update
    void Start()
    {
    }

    // Update is called once per frame
    void Update()
    {
        //get the image point, place it on the projection plane
        Vector3 imagePoint;
        imagePoint.x = this.GetImagePoint().y - (imageWidth-1)/2;
        imagePoint.y = - (this.GetImagePoint().x - (imageHeight - 1)/2);
        imagePoint.z = focalLength;

        //inverse camera rotation to place the point in world space
        Matrix4x4 rotation = Matrix4x4.Rotate(Quaternion.Euler(cameraRotation)).inverse; 
        imagePoint = rotation.MultiplyPoint3x4(imagePoint);

        head.y = trackerHeight - cameraHeight;
        head.x = imagePoint.x * (imagePoint.y / head.y);
        head.z = imagePoint.z * (imagePoint.y / head.y);

        foot.x = head.x;
        foot.y = head.y - trackerHeight;
        foot.z = head.z;

        Debug.DrawLine(Vector3.zero, imagePoint);
        Debug.DrawLine(imagePoint, head);
        Debug.DrawLine(head, foot);

    }
}
