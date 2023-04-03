#pragma once
#include "VulkanHandle.h"
#include "Settings.h"
//#include "Context.h"
//#include "Window.h"


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
//				 -> I dont hate this idea however its bad OOP 
//				 -> but honestly it would reduce alot of boilerplate code 
//				 -> reduces 'pipeline.pool<A>().emplace()' to 'pipeline.emplace<A>()'
//				 -> how would it handle the const vs non const?
//				 -> could do a simple requirement disabling as necessary



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

namespace Gawr::Core {
	class Application {
		DisplaySettings settings;
		//Context context;
		//Window window;

		Application() : 
			settings("Config/displaySettings.cfg") 
			//context(settings), 
			//window(settings) 
		{ }


		// handle window settings config files etc
		
			// swapchain x 1 -> queueFamilies x N
			// queueFamily x 1 -> queue x N
			// queue x 1 -> cmdPool x N
			// cmdPool x 1 -> thread x 1
		
			// swapchain -> a collection of images and information on how to render to them


	};
}
