using UnityEngine;
using UnityEngine.XR.ARFoundation;

// https://docs.unity3d.com/Packages/com.unity.xr.arfoundation@4.2/manual/tracked-image-manager.html

public class ImageTrackHandler : MonoBehaviour
{
    [SerializeField]
    ARTrackedImageManager m_TrackedImageManager;
    [SerializeField] GameObject prefab;
    GameObject origin = null;

    void OnEnable() => m_TrackedImageManager.trackedImagesChanged += OnChanged;

    void OnDisable() => m_TrackedImageManager.trackedImagesChanged -= OnChanged;

    void OnChanged(ARTrackedImagesChangedEventArgs eventArgs)
    {
        foreach (var newImage in eventArgs.added)
        {
            Debug.Log("Added " + newImage.name);
        }

        foreach (var updatedImage in eventArgs.updated)
        {
            Debug.Log(string.Format("{0}\ntrackingState: {1}\nGUID: {2}\nReference size: {3} cm\nDetected size: {4} cm\nPosition: {5}\nRotation: {6}",
            updatedImage.referenceImage.name,
            updatedImage.trackingState,
            updatedImage.referenceImage.guid,
            updatedImage.referenceImage.size * 100f,
            updatedImage.size * 100f,
            updatedImage.transform.position,
            updatedImage.transform.rotation));
            if(!origin)origin = Instantiate(prefab, updatedImage.transform);
        }

        foreach (var removedImage in eventArgs.removed)
        {
            Debug.Log("Lost " + removedImage.name);
        }
    }

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }
}
