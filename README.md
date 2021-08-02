# VkPlayground
Vulkan Playground  
  
This is a self-initiated project where I take a dive into Vulkan (using the C++ bindings) after prior experience in D3D11 to get my hands dirty with a lower-level graphics API.  
  
The purpose of this project is to get an adequate understanding of new concepts such Command Buffers and resource synchronization for CPU-GPU and GPU-GPU and familiarize myself with the API primitives (such as Render Passes) and workflow to get 
typical resources (e.g Buffers/Textures) to the GPU and use them. It was also a good opportunity to get to know other widely used libraries such as GLFW for Window/Input handling and GLM for maths which I otherwise don't use when working with D3D11.  
  
The flow of logic can be found in the "SponzaApp". The only complex part of this project is the VulkanContext which has all the initialization code and also an abstraction for "beginFrame" and "endFrame"  
to aid myself in WSI frame synchronization. Fancy abstractions was outside the scope of this project. I wanted to interface with Vulkan as much as possible to get a better feel for the workflow.  
  
Miscellaneous Vulkan-related work done:
- Manual mip generation for loaded textures.  
- Manual setup for optimal device local buffer (e.g Vertex/Index buffer) through staging buffer copy (textures are uploaded this way too)  
- One-time GPU submit work abstraction (for miscellaneous one-time work like above, e.g copy buffer to buffer on GPU).  
- Setup per-frame resources to allow multiple GPU frames-in-flight (e.g a Command Buffer per frame).  
- Use dynamic buffer offsets to use a single buffer for multiple structures and for multiple frames in flight (Engine/Scene data in code).  
- Have proper image layout transitions for texture creation and WSI sync.
- Enable alpha-to-coverage to use opacity masks.
- Multiple meshes in a single buffer, accessed and rendered through vertex offsets.
- ImGui hook
  
Basic tech implemented in this application:  
- Assimp model loading (With diffuse, opacity, specular and normal textures supported)
- Phong Lighting (Blinn Phong specular)
- Directional Light
- Spotlight
- Point Light
- Normal mapping
- Skybox
  
I unfortunately have no complex synchronization usage which I can showcase in this project which would be required for, e.g, Deferred Rendering (Geometry Pass, then Light Pass) where resource dependencies are needed (between both subpass)
to avoid Read-After-Write hazards on the GBuffers. 
  
All-in-all, it was a good experience to work with a modern graphics API. I have gotten a taste of the workflow and the primitives a modern graphics API provides and made a basic application! I have also gained a better appreciation of a higher level graphics API and understand, at least partly, what it may do for me under-the-hood.    
  
Although Vulkan has been interesting, I think that it is very time-consuming to work with. There is a lot more information to read up on and variations to consider when making abstractions for the API.  
I have come to realize that the control (and complexity) of Vulkan is not something I want to prioritize right now and I will put aside this project for now and revisit a lower-level graphics API again in the near future when it is more appropriate. :)  
  
General
![](Animation.gif)  
  
![Alt text](pic1.png?raw=true "General")
  
Normal mapping  
![Alt text](nor.png?raw=true "Normal")
  
Skybox
![Alt text](skybox.png?raw=true "Skybox")


