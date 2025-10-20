#pragma once

#include "core/InputHandler.h"
#include "core/FPSCamera.h"
#include "core/WindowManager.hpp"
#include "core/ShaderProgramManager.hpp"
#include "core/helpers.hpp"
#include "core/Bonobo.h"
#include "core/node.hpp"
#include "torus_game_data.hpp"
#include "torus_path.hpp"

#include <vector>
#include <chrono>
#include <memory>

// Game states
enum class GameStateEnum {
    NEW_GAME,
    PLAY_GAME,
    END_GAME
};

class Window;


namespace edaf80
{
	//! \brief Torus Ride Game class
	class TorusRideGame {
	public:
		TorusRideGame(WindowManager& windowManager, InputHandler& inputHandler, FPSCameraf& camera);
		~TorusRideGame();

		void update(std::chrono::microseconds deltaTime);
		void render(glm::mat4 const& view_projection);
		void renderUI();

	private:
		void initializeShaders();
		void initializeGeometry();
        void initializeTestRings();
        void generateSimpleLineRings();
        void generateInfiniteRings();
		
		// Game logic functions
		void handleInput();
		void updateShipPhysics(std::chrono::microseconds deltaTime);
		void checkRingCollisions();
		void updateCamera();
		
		// Game state
		GameStateEnum currentState;
		
		// Game data
		std::vector<Ring> rings;
		Ship ship;
		GameState gameState;
		GameParams gameParams;
		
		// Path generation
		torus_path::PathGenerator pathGenerator;
		
        // Rendering
        ShaderProgramManager programManager;
        GLuint fallbackShader;
        GLuint torusBasicShader;
        GLuint skyboxShader;
        GLuint skyboxTexture;
        GLuint ringTexture;
		
        // Geometry
        bonobo::mesh_data torusMesh;
        bonobo::mesh_data shipMesh;
        bonobo::mesh_data pathLineMesh;
        bonobo::mesh_data skyboxMesh;
		
		// Simple rendering - no complex node system for now
		// bonobo::Node torusNode;
		// bonobo::Node shipNode;
		
	// References to external systems
	WindowManager& windowManager;
	InputHandler& inputHandler;
	FPSCameraf& camera;
		
		// UI state
		bool showDebugInfo;
		bool showControls;
	};

	//! \brief Wrapper class for Assignment 5
	class Assignment5 {
	public:
		//! \brief Default constructor.
		//!
		//! It will initialise various modules of bonobo and retrieve a
		//! window to draw to.
		Assignment5(WindowManager& windowManager);

		//! \brief Default destructor.
		//!
		//! It will release the bonobo modules initialised by the
		//! constructor, as well as the window.
		~Assignment5();

		//! \brief Contains the logic of the assignment, along with the
		//! render loop.
		void run();

	private:
		FPSCameraf     mCamera;
		InputHandler   inputHandler;
		WindowManager& mWindowManager;
		GLFWwindow*    window;
		
		// Game instance
		std::unique_ptr<TorusRideGame> game;
	};
}
