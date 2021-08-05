# VkPlayground
Vulkan Playground  
  
This is a self-initiated project where I take a dive into Vulkan (using the C++ bindings) after prior experience with D3D11 to get my hands dirty with a lower-level graphics API.
  
I will be refactoring as I go, when I need it, as I extend this application. It is worth mentioning that my primary focus with this project is to explore Vulkan. I am in no way trying to architect a well modularized engine so there will probably be
redundant dependencies or tightly coupled modules around.
  
Basic tech implemented so far
- Assimp model loading (With diffuse, opacity, specular and normal textures supported)
- Phong Lighting (with Blinn Phong specular)
- Directional Light (not in any screenshots)
- Spotlight
- Point Light
- Normal mapping
- Skybox
  
References:
- [Synchronization (TheMaister)](https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/)
- [Synchronization (Jeremy Ong)](https://www.jeremyong.com/vulkan/graphics/rendering/2018/11/22/vulkan-synchronization-primer/)
- [Vulkan-Tutorial (Alexander Overvoorde)](https://vulkan-tutorial.com/)
- [VkGuide (Victor Blanco)](https://vkguide.dev/)
- [API Without Secrets (Pawel Lapinski)](https://software.intel.com/content/www/us/en/develop/articles/api-without-secrets-introduction-to-vulkan-preface.html)
- [Minimal Vulkan HPP (Daniel Elliott)](https://github.com/dokipen3d/vulkanHppMinimalExample)
- [Introduction To Vulkan (Johannes Unterguggenberger)](https://www.youtube.com/watch?v=isbMMIwmZes)
- [Design Patterns for Low-Level Real-Time Rendering (Nicolas Guillemot)](https://www.youtube.com/watch?v=mdPeXJ0eiGc)
- [Vulkan Spec](https://renderdoc.org/vkspec_chunked/index.html)
  
General
![](Animation.gif)  
  
![Alt text](pic1.png?raw=true "General")
  
Normal mapping  
![Alt text](nor.png?raw=true "Normal")
  
Skybox
![Alt text](skybox.png?raw=true "Skybox")


