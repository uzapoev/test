using UnityEngine;
using UnityEditor;
using System.Collections.Generic;
using System.Linq;

namespace UnityJsonExporter
{

  public static class ColliderExporter
  {
    private static int GetColliderType(Collider collider)
    {
      return collider switch
      {
        BoxCollider    => 0,
        SphereCollider => 1,
        CapsuleCollider => 2,
        MeshCollider   => 3,
        _              => 99 // unknown
      };
    }

    public static void ExportCollider(this Exporter exporter, Collider _collider, SceneNode node)
    {
      if (_collider == null)
        return;

      var colliders = _collider.gameObject.GetComponents<Collider>();
      if (colliders == null || colliders.Length == 0)
        return;

      var collider = colliders[0];

      node.collider = new ColliderComponent { type = GetColliderType(collider), isTrigger = collider.isTrigger };

      switch (collider)
      {
        case BoxCollider box:
          node.collider.center = box.center;
          node.collider.size = box.size;
          break;

        case SphereCollider sphere:
          node.collider.center = sphere.center;
          node.collider.radius = sphere.radius;
          break;

        case CapsuleCollider capsule:
          node.collider.center = capsule.center;
          node.collider.radius = capsule.radius;
          node.collider.height = capsule.height;
          node.collider.direction = capsule.direction;
          break;

        case MeshCollider meshCollider:
          if (meshCollider.sharedMesh != null)
          {
            exporter.ExportMesh(meshCollider.sharedMesh);   // the referenced mesh has to exist as a file and be in the scene list
            var meshGuid = exporter.GetOrCreateMeshGuid(meshCollider.sharedMesh);
            node.collider.meshGuid = meshGuid;
          }

          node.collider.convex = meshCollider.convex;
          break;
      }
    }
  }
}