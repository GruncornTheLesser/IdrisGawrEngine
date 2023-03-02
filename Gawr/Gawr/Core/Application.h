#pragma once
#include "VulkanHandle.h"


// ToDo: 
// ECS -> event system
//		issue -> the event will have component access requirements
//		solution -> group events with same access requirements and execute in parallel
//				 -> reduces the overhead of many mutex locks & unlocks 
//				 -> issues trying to invoke events as thread will need to join
//		
//		solution -> the pipeline could implement the pool event dispatchers
//				 -> allowing events to have the information about how they were executed
//				 -> and therefore trying accessing already unlocked data from the same thread
// 
//		solution -> create a synchronisation for all events and execute events in serial after access is released
//				 -> has a larger impact on design than I would like
// 
//		solution -> when a pipeline acquires write access to a pool, it can store a pointer in the pool
//				 -> this runs into issues from type erasure as the pipeline is a template object
//				 -> overall it would be messier than I would like
//				 -> it would work something like this:
//				 -> add an extra template argument to pool and reinterpret the reference when returning from pipeline with the added 
//				 -> pointer type
// 
//		solution -> move all pool functionality to the pipeline
//

// Designing the Application Structure:
//		
// Components
// VBOs: Index/Position/Colour/UV
// Material: Albedo/Normal/UV/Texture
// Hierarchy: Parent
// Transform : World/Local/Position/Rotation/Scale
// script: onCreate(), onUpdate(), onDestroy()???
// Animator
// LightEmitter
// ShadowCaster<Mode>: Mode = Projection/Mapping
// 



class ShaderProgram;

class DisplaySettings; // window size, display mode, 

class AppSettings; // app name, graphics mode, use terminal?? idk

class Application {
	class Graphics {
		class Context {
			// instance
			// physicalDevice
			// logicalDevice
		};

		class Swapchain {
			// swapchain
			// images
			// imageviews
			// framebuffers
		};

		class Renderer {
			// commandBuffer
			// semaphores
			// fences
		};
	};


};
