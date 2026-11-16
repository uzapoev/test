using UnityEngine;
using UnityEditor;
using System.Collections.Generic;
using System.Linq;

namespace UnityJsonExporter
{
  public static class LightExporter
  {
    public static void ExportLight(this Exporter exporter, Light light, SceneNode node)
    {
      if (light == null)
        return;

     // light.lightmapBakeType = LightmapBakeType.None;

      var lightComponent = new LightComponent
      {
        color = light.color,
        intensity = light.intensity,
        radius = light.range,

        mode = light.type switch
        {
          LightType.Directional => 0,
          LightType.Point => 1,
          LightType.Spot => 2,
          LightType.Rectangle => 3,
          _ => 0
        },

        dir = (light.type == LightType.Directional || light.type == LightType.Spot)
          ? light.transform.forward
          : Vector3.zero,

        shadowMode = light.shadows switch
        {
          LightShadows.None => 0,
          LightShadows.Hard => 1,
          LightShadows.Soft => 2,
          _ => 0
        }
      };

      node.light = lightComponent;
    }
  }
}