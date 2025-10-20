#include "assignment5.hpp"
#include "parametric_shapes.hpp"
#include "torus_game_data.hpp"
#include "torus_path.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/ShaderProgramManager.hpp"
#include "core/Log.h"

#include <imgui.h>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// TorusRideGame implementation
edaf80::TorusRideGame::TorusRideGame(WindowManager& windowManager, InputHandler& inputHandler, FPSCameraf& camera)
    : windowManager(windowManager), inputHandler(inputHandler), camera(camera),
      currentState(GameStateEnum::NEW_GAME),
      fallbackShader(0u), torusBasicShader(0u), skyboxShader(0u), skyboxTexture(0u),
      showDebugInfo(true), showControls(true)
{
    initializeShaders();
    initializeGeometry();
    initializeTestRings();
}

edaf80::TorusRideGame::~TorusRideGame()
{
    // Cleanup will be handled by bonobo framework
}

void edaf80::TorusRideGame::initializeShaders()
{
    // Register fallback shader
    programManager.CreateAndRegisterProgram("Fallback",
        { { ShaderType::vertex, "common/fallback.vert" },
          { ShaderType::fragment, "common/fallback.frag" } },
        fallbackShader);
    
    if (fallbackShader == 0u) {
        LogError("Failed to load fallback shader");
        return;
    }
    
    // Register torus basic shader
    programManager.CreateAndRegisterProgram("Torus Basic",
        { { ShaderType::vertex, "EDAF80/torus_basic.vert" },
          { ShaderType::fragment, "EDAF80/torus_basic.frag" } },
        torusBasicShader);
    
    if (torusBasicShader == 0u) {
        LogError("Failed to load torus basic shader");
        return;
    }
    
    // Register skybox shader like assignment 4
    programManager.CreateAndRegisterProgram("Skybox",
        { { ShaderType::vertex, "EDAF80/skybox.vert" },
          { ShaderType::fragment, "EDAF80/skybox.frag" } },
        skyboxShader);
    
    if (skyboxShader == 0u) {
        LogWarning("Failed to load skybox shader, will skip skybox rendering");
    }
    
    LogInfo("TorusRideGame shaders initialized successfully");
}

void edaf80::TorusRideGame::initializeGeometry()
{
    // Create torus mesh for rings with better tessellation
    torusMesh = parametric_shapes::createTorus(gameParams.ring_R, gameParams.ring_r, 32u, 16u);
    if (torusMesh.vao == 0u) {
        LogError("Failed to create torus mesh");
        return;
    }
    
    // Create ship mesh (elongated sphere for better direction indication)
    shipMesh = parametric_shapes::createSphere(0.5f, 16u, 8u);
    if (shipMesh.vao == 0u) {
        LogError("Failed to create ship mesh");
        return;
    }
    
    // Load textures if available
    GLuint shipTexture = 0u;
    
    // Try to load ship texture (if it exists)
    try {
        shipTexture = bonobo::loadTexture2D(config::resources_path("textures/ship_diffuse.jpg"), true);
        if (shipTexture != 0u) {
            LogInfo("Loaded ship texture successfully");
        }
    } catch (...) {
        LogWarning("Could not load ship texture, using default material");
    }
    
    // Try to load ring texture (if it exists)
    try {
        ringTexture = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_coll1_2k.jpg"), true);
        if (ringTexture != 0u) {
            LogInfo("Loaded ring texture successfully");
        }
    } catch (...) {
        LogWarning("Could not load ring texture, using default material");
    }
    
    // Simplified geometry setup - no complex node system for now
    // Just store the mesh data for basic rendering
    
    // Create skybox mesh like assignment 4
    skyboxMesh = parametric_shapes::createSphere(500.0f, 100u, 100u);
    if (skyboxMesh.vao == 0u) {
        LogError("Failed to create skybox mesh");
    }
    
    // Load skybox texture like assignment 4
    skyboxTexture = bonobo::loadTextureCubeMap(
        config::resources_path("cubemaps/NissiBeach2/posx.jpg"),
        config::resources_path("cubemaps/NissiBeach2/negx.jpg"),
        config::resources_path("cubemaps/NissiBeach2/posy.jpg"),
        config::resources_path("cubemaps/NissiBeach2/negy.jpg"),
        config::resources_path("cubemaps/NissiBeach2/posz.jpg"),
        config::resources_path("cubemaps/NissiBeach2/negz.jpg"),
        true
    );
    
    if (skyboxTexture == 0u) {
        LogWarning("Failed to load skybox texture");
    } else {
        LogInfo("Skybox texture loaded successfully");
    }
    
    // Initialize some test rings for demonstration
    initializeTestRings();
    
    LogInfo("TorusRideGame geometry initialized successfully");
    LogInfo("- Torus mesh: %u vertices, %u indices", torusMesh.vertices_nb, torusMesh.indices_nb);
    LogInfo("- Ship mesh: %u vertices, %u indices", shipMesh.vertices_nb, shipMesh.indices_nb);
    LogInfo("- Skybox mesh: %u vertices, %u indices", skyboxMesh.vertices_nb, skyboxMesh.indices_nb);
    LogInfo("- Test rings initialized: %zu rings", rings.size());
}

void edaf80::TorusRideGame::initializeTestRings()
{
    // Create control points for a curved path
    std::vector<glm::vec3> controlPoints = {
        glm::vec3(0.0f, 0.0f, 0.0f),      // Start
        glm::vec3(5.0f, 2.0f, -10.0f),    // First curve
        glm::vec3(10.0f, 0.0f, -20.0f),   // Second curve
        glm::vec3(15.0f, -3.0f, -30.0f),  // Third curve
        glm::vec3(20.0f, 0.0f, -40.0f),   // Fourth curve
        glm::vec3(15.0f, 5.0f, -50.0f),   // Fifth curve
        glm::vec3(10.0f, 0.0f, -60.0f),   // Sixth curve
        glm::vec3(5.0f, -2.0f, -70.0f),   // Seventh curve
        glm::vec3(0.0f, 0.0f, -80.0f)     // End
    };
    
    // Generate the path
    pathGenerator.generate_path(controlPoints, 1000);
    
    if (!pathGenerator.is_valid()) {
        LogError("Failed to generate path, falling back to simple line");
        generateSimpleLineRings();
        return;
    }
    
    // Generate rings along the path
    rings.clear();
    float totalLength = pathGenerator.get_total_length();
    int numRings = static_cast<int>(totalLength / gameParams.ring_spacing);
    rings.reserve(numRings);
    
    LogInfo("Generating %d rings along path of length %.2f", numRings, totalLength);
    
    for (int i = 0; i < numRings; ++i) {
        Ring ring;
        
        // Calculate arc length for this ring
        float s = i * gameParams.ring_spacing;
        
        // Get position and frame from path
        glm::vec3 position = pathGenerator.get_position(s);
        torus_path::Frame frame = pathGenerator.get_frame(s);
        
        // Create world matrix for the ring
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotate = glm::mat4(1.0f);
        
        // Set up rotation matrix from frame
        rotate[0] = glm::vec4(frame.B, 0.0f);  // Right
        rotate[1] = glm::vec4(frame.N, 0.0f);  // Up
        rotate[2] = glm::vec4(frame.T, 0.0f);  // Forward
        rotate[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        
        ring.world = translate * rotate;
        ring.center = position;
        
        // Set ring axis (tangent direction)
        ring.axis = frame.T;
        
        // Set ring dimensions
        ring.R = gameParams.ring_R;
        ring.r = gameParams.ring_r;
        
        // Initialize state
        ring.passed = false;
        ring.scored = false;
        
        rings.push_back(ring);
    }
    
    LogInfo("Generated %zu rings along curved path", rings.size());
}

void edaf80::TorusRideGame::generateSimpleLineRings()
{
    // Generate only 3 rings that are perpendicular to Z-axis
    rings.clear();
    rings.reserve(3);
    
    // Generate only 3 rings in front of the ship with sin wave pattern
    // Increased spacing for more reaction time
    float spacing = gameParams.ring_spacing * 2.0f;  // Double the spacing
    
    for (int i = 0; i < 3; ++i) {
        Ring ring;
        
        // Position rings along Z-axis with sin wave pattern in XY plane
        float z = ship.position.z - 15.0f - i * spacing;  // Start ahead of ship with more spacing
        float x = 4.0f * sin(z * 0.3f);  // Sin wave in X direction
        float y = 3.0f * sin(z * 0.2f);  // Sin wave in Y direction with different frequency
        
        glm::vec3 position = glm::vec3(x, y, z);
        
        // Create world matrix for the ring - rotate to be perpendicular to Z-axis
        ring.world = glm::translate(glm::mat4(1.0f), position);
        ring.world = glm::rotate(ring.world, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate to be perpendicular to Z
        
        ring.center = position;
        
        // Set ring axis (perpendicular to Z-axis)
        ring.axis = glm::vec3(0.0f, 0.0f, 1.0f);
        
        // Set ring dimensions
        ring.R = gameParams.ring_R;
        ring.r = gameParams.ring_r;
        
        // Initialize state
        ring.passed = false;
        ring.scored = false;
        
        rings.push_back(ring);
    }
    
    LogInfo("Generated %zu rings with sin wave pattern (3 rings only)", rings.size());
}

void edaf80::TorusRideGame::generateInfiniteRings()
{
    // Always maintain exactly 3 rings visible
    if (rings.empty()) {
        generateSimpleLineRings();
        return;
    }
    
    // Remove rings that have been passed by the ship
    rings.erase(
        std::remove_if(rings.begin(), rings.end(),
            [this](const Ring& ring) {
                return ring.center.z > ship.position.z + 5.0f; // Remove rings behind ship
            }),
        rings.end()
    );
    
    // Always ensure we have exactly 3 rings
    while (rings.size() < 3) {
        Ring ring;
        
        // Find the furthest ring to place the new one ahead of it
        float furthestZ = ship.position.z - 15.0f; // Start position
        if (!rings.empty()) {
            furthestZ = rings[0].center.z;
            for (const auto& r : rings) {
                if (r.center.z < furthestZ) {
                    furthestZ = r.center.z;
                }
            }
        }
        
        // Place new ring ahead of the furthest one
        float spacing = gameParams.ring_spacing * 2.0f;  // Double the spacing
        float z = furthestZ - spacing;
        float x = 4.0f * sin(z * 0.3f);
        float y = 3.0f * sin(z * 0.2f);
        
        glm::vec3 position = glm::vec3(x, y, z);
        
        ring.world = glm::translate(glm::mat4(1.0f), position);
        ring.world = glm::rotate(ring.world, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate to be perpendicular to Z
        ring.center = position;
        ring.axis = glm::vec3(0.0f, 0.0f, 1.0f);
        ring.R = gameParams.ring_R;
        ring.r = gameParams.ring_r;
        ring.passed = false;
        ring.scored = false;
        
        rings.push_back(ring);
    }
    
    // Sort rings by Z position (furthest to closest)
    std::sort(rings.begin(), rings.end(),
        [](const Ring& a, const Ring& b) {
            return a.center.z < b.center.z; // Closest first
        });
}

void edaf80::TorusRideGame::update(std::chrono::microseconds deltaTime)
{
    // Input events
    auto& io = ImGui::GetIO();
    
    // Priority check: Handle R key reset in END_GAME state first
    // This must be done BEFORE setting UI capture to ensure R key works
    if (currentState == GameStateEnum::END_GAME) {
        if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
            LogInfo("=== RESET GAME PRESSED ===");
            currentState = GameStateEnum::NEW_GAME;
            return; // Return early to avoid processing other logic
        }
        // Debug: Log that we're in END_GAME state and checking for R key
        static int debugCounter = 0;
        if (++debugCounter % 60 == 0) { // Log every 60 frames (about 1 second at 60fps)
            LogInfo("END_GAME state: Waiting for R key press. R key state: %d", 
                    inputHandler.GetKeycodeState(GLFW_KEY_R));
        }
    }
    
    // Set UI capture after handling critical game input
    inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);
    inputHandler.Advance();
    
    // Update camera with mouse input only (for looking around)
    camera.Update(deltaTime, inputHandler);
    
    float dt = std::chrono::duration<float>(deltaTime).count();
    
    // Game state machine
    switch (currentState) {
        case GameStateEnum::NEW_GAME:
            // Do first time setup of variables here
            // Prepare for a new round
            gameState = GameState(); // Reset game state
            ship = Ship(); // Reset ship
            
            // Clear all rings and regenerate them
            rings.clear();
            generateSimpleLineRings();
            
            // Reset camera position
            camera.mWorld.SetTranslate(glm::vec3(0.0f, 5.0f, 15.0f));
            camera.mWorld.LookAt(glm::vec3(0.0f, 0.0f, 0.0f));
            
            LogInfo("Game reset complete - starting new game");
            currentState = GameStateEnum::PLAY_GAME;
            break;
            
        case GameStateEnum::PLAY_GAME:
            // Game logic here
            // Control input, physics update, render
            handleInput();
            updateShipPhysics(deltaTime);
            checkRingCollisions();
            generateInfiniteRings();  // Generate new rings as needed
            
            // Game over is now handled directly in collision detection
            // No need to check misses count since any miss = immediate game over
            break;
            
        case GameStateEnum::END_GAME:
            // Deal with showing high-scores
            // R key is handled before the switch statement
            break;
    }
    
    // Always update camera in all states
    updateCamera();
    
    // Update game time
    gameState.play_time += dt;
}

void edaf80::TorusRideGame::handleInput()
{
    float dt = 0.016f; // Assume 60 FPS for smooth controls
    
    // Simple XY plane movement control
    float moveSpeed = 5.0f * dt;
    
    // W/S - Up/Down movement
    if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED) {
        ship.position.y += moveSpeed;
    }
    if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED) {
        ship.position.y -= moveSpeed;
    }
    
    // A/D - Left/Right movement
    if (inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED) {
        ship.position.x -= moveSpeed;
    }
    if (inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED) {
        ship.position.x += moveSpeed;
    }
}

void edaf80::TorusRideGame::updateShipPhysics(std::chrono::microseconds deltaTime)
{
    float dt = std::chrono::duration<float>(deltaTime).count();
    
    // Simple forward movement - ship always moves forward in Z direction
    ship.forward = glm::vec3(0.0f, 0.0f, -1.0f);  // Always pointing forward
    ship.position.z -= ship.speed_current * dt;    // Move forward in Z direction
    
    // XY movement is handled in handleInput()
}

void edaf80::TorusRideGame::checkRingCollisions()
{
    for (auto& ring : rings) {
        if (ring.passed || ring.scored) continue;
        
        // Calculate XY plane distance only (ring is perpendicular to Z axis)
        glm::vec2 shipXY = glm::vec2(ship.position.x, ship.position.y);
        glm::vec2 ringXY = glm::vec2(ring.center.x, ring.center.y);
        float distanceXY = glm::length(shipXY - ringXY);
        
        // Check if ship is close to ring plane - increased range for better detection
        float zDistance = std::abs(ship.position.z - ring.center.z);
        bool nearPlane = zDistance < 5.0f; // Increased from 3.0f to 5.0f
        
        // Check if ring was passed without scoring (miss) - GAME OVER
        // Only check this if ship is clearly past the ring
        if (ship.position.z < ring.center.z - 3.0f && !ring.scored && !ring.passed) {
            LogInfo("GAME OVER! Missed ring - ship passed without going through hole (Z: %.2f < Ring Z: %.2f - 3.0)", 
                    ship.position.z, ring.center.z);
            currentState = GameStateEnum::END_GAME;
            break;
        }
        
        // Check collision when near the ring plane
        if (nearPlane) {
            // Calculate hole radius: ring_R - ring_r
            float holeRadius = ring.R - ring.r;
            
            // Ship center must be in hole to score (XY distance < hole radius)
            if (distanceXY < holeRadius) {
                if (!ring.scored) {
                    ring.scored = true;
                    gameState.score += 100;
                    gameState.combo++;
                    LogInfo("SCORED! XY Distance: %.2f < Hole radius: %.2f (Z dist: %.2f)", 
                            distanceXY, holeRadius, zDistance);
                }
            }
            // Check if ship hit the torus (holeRadius < distanceXY < ring.R)
            else if (distanceXY < ring.R) {
                LogInfo("GAME OVER! Hit torus - XY Distance: %.2f, not in hole (%.2f) but hit torus (%.2f)", 
                        distanceXY, holeRadius, ring.R);
                currentState = GameStateEnum::END_GAME;
                break;
            }
            // Ship outside the ring completely (distanceXY >= ring.R) - this is OK if not past the ring yet
            // Only game over if ship is past the ring and outside it
            else if (ship.position.z < ring.center.z - 1.0f) {
                LogInfo("GAME OVER! Outside ring - XY Distance: %.2f >= Ring radius: %.2f and past ring", 
                        distanceXY, ring.R);
                currentState = GameStateEnum::END_GAME;
                break;
            }
            // If ship is outside ring but not past it yet, just continue (no action needed)
        }
    }
}

void edaf80::TorusRideGame::updateCamera()
{
    // Camera follows the ship from behind and slightly above
    static glm::vec3 cameraPosition = glm::vec3(0.0f, 5.0f, 15.0f);  // Initial camera position
    static glm::vec3 targetLookAt = glm::vec3(0.0f, 0.0f, 0.0f);     // Initial look at position
    static bool needsReset = false;
    
    // Reset camera on game restart
    if (currentState == GameStateEnum::NEW_GAME) {
        cameraPosition = glm::vec3(0.0f, 5.0f, 15.0f);
        targetLookAt = glm::vec3(0.0f, 0.0f, 0.0f);
        needsReset = true;
    }
    
    // Calculate desired camera position (behind and above the ship)
    glm::vec3 desiredCameraPosition = ship.position - ship.forward * 8.0f + glm::vec3(0.0f, 3.0f, 0.0f);
    glm::vec3 desiredLookAt = ship.position;
    
    // Smooth interpolation for camera movement
    float lerpFactor = needsReset ? 1.0f : 0.1f;  // Instant reset or smooth follow
    cameraPosition = glm::mix(cameraPosition, desiredCameraPosition, lerpFactor);
    targetLookAt = glm::mix(targetLookAt, desiredLookAt, lerpFactor);
    needsReset = false;
    
    // Update camera
    camera.mWorld.SetTranslate(cameraPosition);
    camera.mWorld.LookAt(targetLookAt);
}

void edaf80::TorusRideGame::render(glm::mat4 const& view_projection)
{
    // Set up basic OpenGL state
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Render skybox first (like assignment 4)
    if (skyboxMesh.vao != 0u && skyboxShader != 0u && skyboxTexture != 0u) {
        glDepthMask(GL_FALSE); // Disable depth writing for skybox
        glUseProgram(skyboxShader);
        
        // Set skybox uniforms
        GLint model_location = glGetUniformLocation(skyboxShader, "vertex_model_to_world");
        GLint vp_location = glGetUniformLocation(skyboxShader, "vertex_world_to_clip");
        
        glm::mat4 skyboxModel = glm::mat4(1.0f);
        glm::mat4 skyboxVP = view_projection;
        
        if (model_location >= 0) {
            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(skyboxModel));
        }
        if (vp_location >= 0) {
            glUniformMatrix4fv(vp_location, 1, GL_FALSE, glm::value_ptr(skyboxVP));
        }
        
        // Bind cubemap texture
        GLint cubemap_location = glGetUniformLocation(skyboxShader, "cubemap");
        if (cubemap_location >= 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            glUniform1i(cubemap_location, 0);
        }
        
        // Draw skybox
        glBindVertexArray(skyboxMesh.vao);
        glDrawElements(GL_TRIANGLES, skyboxMesh.indices_nb, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0u);
        
        glDepthMask(GL_TRUE); // Re-enable depth writing
    }
    
    // Set up lighting
    glm::vec3 lightPos = glm::vec3(10.0f, 10.0f, 10.0f);
    glm::vec3 lightColor = glm::vec3(1.0f);
    glm::vec3 cameraPos = camera.mWorld.GetTranslation();
    
    // Render game objects using mesh data if available
    if (torusMesh.vao != 0u && torusBasicShader != 0u) {
        glUseProgram(torusBasicShader);
        
        // Set lighting uniforms
        GLint light_pos_loc = glGetUniformLocation(torusBasicShader, "light_position");
        GLint light_color_loc = glGetUniformLocation(torusBasicShader, "light_color");
        GLint camera_pos_loc = glGetUniformLocation(torusBasicShader, "camera_position");
        GLint env_map_loc = glGetUniformLocation(torusBasicShader, "environment_map");
        GLint has_diffuse_tex_loc = glGetUniformLocation(torusBasicShader, "has_diffuse_texture");
        
        if (light_pos_loc >= 0) glUniform3fv(light_pos_loc, 1, glm::value_ptr(lightPos));
        if (light_color_loc >= 0) glUniform3fv(light_color_loc, 1, glm::value_ptr(lightColor));
        if (camera_pos_loc >= 0) glUniform3fv(camera_pos_loc, 1, glm::value_ptr(cameraPos));
        if (has_diffuse_tex_loc >= 0) glUniform1i(has_diffuse_tex_loc, 1); // rings: use diffuse texture
        if (env_map_loc >= 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            glUniform1i(env_map_loc, 1);
        }
        
        // Bind ring texture
        GLint diffuse_tex_loc = glGetUniformLocation(torusBasicShader, "diffuse_texture");
        if (diffuse_tex_loc >= 0 && ringTexture != 0u) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ringTexture);
            glUniform1i(diffuse_tex_loc, 0);
        }
        
        // Render rings using the torus mesh
        for (const auto& ring : rings) {
            // Set up transformation matrix
            glm::mat4 model = ring.world;
            glm::mat4 mvp = view_projection * model;
            
            // Set uniforms for the shader
            GLint mvp_location = glGetUniformLocation(torusBasicShader, "vertex_world_to_clip");
            GLint model_location = glGetUniformLocation(torusBasicShader, "vertex_model_to_world");
            GLint normal_location = glGetUniformLocation(torusBasicShader, "normal_model_to_world");
            
            if (mvp_location >= 0) {
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(view_projection));
            }
            if (model_location >= 0) {
                glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));
            }
            if (normal_location >= 0) {
                glm::mat4 normal_matrix = glm::transpose(glm::inverse(model));
                glUniformMatrix4fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));
            }
            
            // Set material properties for rings (green)
            GLint diffuse_loc = glGetUniformLocation(torusBasicShader, "diffuse_color");
            GLint specular_loc = glGetUniformLocation(torusBasicShader, "specular_color");
            GLint shininess_loc = glGetUniformLocation(torusBasicShader, "shininess");
            GLint emissive_loc = glGetUniformLocation(torusBasicShader, "emissive_color");
            
            if (diffuse_loc >= 0) glUniform3f(diffuse_loc, 1.0f, 1.0f, 1.0f); // White to show texture
            if (specular_loc >= 0) glUniform3f(specular_loc, 1.0f, 1.0f, 1.0f); // stronger specular for visible env reflection
            if (shininess_loc >= 0) glUniform1f(shininess_loc, 64.0f); // sharper highlight
            if (emissive_loc >= 0) glUniform3f(emissive_loc, 0.0f, 0.0f, 0.0f); // remove emissive to not wash out reflection
            
            // Draw the torus
            glBindVertexArray(torusMesh.vao);
            glDrawElements(GL_TRIANGLES, torusMesh.indices_nb, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0u);
        }
    }
    
    if (shipMesh.vao != 0u && torusBasicShader != 0u) {
        glUseProgram(torusBasicShader);
        
        // Set lighting uniforms for ship
        GLint light_pos_loc = glGetUniformLocation(torusBasicShader, "light_position");
        GLint light_color_loc = glGetUniformLocation(torusBasicShader, "light_color");
        GLint camera_pos_loc = glGetUniformLocation(torusBasicShader, "camera_position");
        GLint env_map_loc = glGetUniformLocation(torusBasicShader, "environment_map");
        GLint has_diffuse_tex_loc = glGetUniformLocation(torusBasicShader, "has_diffuse_texture");
        
        if (light_pos_loc >= 0) glUniform3fv(light_pos_loc, 1, glm::value_ptr(lightPos));
        if (light_color_loc >= 0) glUniform3fv(light_color_loc, 1, glm::value_ptr(lightColor));
        if (camera_pos_loc >= 0) glUniform3fv(camera_pos_loc, 1, glm::value_ptr(cameraPos));
        if (has_diffuse_tex_loc >= 0) glUniform1i(has_diffuse_tex_loc, 0); // ship: no diffuse texture bound
        if (env_map_loc >= 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            glUniform1i(env_map_loc, 1);
        }
        
        // Render ship using the ship mesh
        glm::mat4 shipTransform = glm::translate(glm::mat4(1.0f), ship.position);
        shipTransform = glm::scale(shipTransform, glm::vec3(0.5f, 0.3f, 1.0f));
        glm::mat4 mvp = view_projection * shipTransform;
        
        // Set uniforms for the shader
        GLint mvp_location = glGetUniformLocation(torusBasicShader, "vertex_world_to_clip");
        GLint model_location = glGetUniformLocation(torusBasicShader, "vertex_model_to_world");
        GLint normal_location = glGetUniformLocation(torusBasicShader, "normal_model_to_world");
        
        if (mvp_location >= 0) {
            glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(view_projection));
        }
        if (model_location >= 0) {
            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(shipTransform));
        }
        if (normal_location >= 0) {
            glm::mat4 normal_matrix = glm::transpose(glm::inverse(shipTransform));
            glUniformMatrix4fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));
        }
        
        // Set material properties for ship (red)
        GLint diffuse_loc = glGetUniformLocation(torusBasicShader, "diffuse_color");
        GLint specular_loc = glGetUniformLocation(torusBasicShader, "specular_color");
        GLint shininess_loc = glGetUniformLocation(torusBasicShader, "shininess");
        GLint emissive_loc = glGetUniformLocation(torusBasicShader, "emissive_color");
        
        if (diffuse_loc >= 0) glUniform3f(diffuse_loc, 0.8f, 0.2f, 0.2f); // Red
        if (specular_loc >= 0) glUniform3f(specular_loc, 1.0f, 1.0f, 1.0f);
        if (shininess_loc >= 0) glUniform1f(shininess_loc, 64.0f);
        if (emissive_loc >= 0) glUniform3f(emissive_loc, 0.1f, 0.0f, 0.0f); // Slight red glow
        
        // Draw the ship
        glBindVertexArray(shipMesh.vao);
        glDrawElements(GL_TRIANGLES, shipMesh.indices_nb, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0u);
    }
}

void edaf80::TorusRideGame::renderUI()
{
    // Debug info panel
    if (showDebugInfo) {
        bool opened = ImGui::Begin("Torus Ride Debug", &showDebugInfo);
        if (opened) {
            ImGui::Text("=== Ship Info ===");
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", ship.position.x, ship.position.y, ship.position.z);
            ImGui::Text("Speed: %.2f", ship.speed_current);
            
            ImGui::Separator();
            ImGui::Text("=== Controls ===");
            ImGui::Text("W: Move Up");
            ImGui::Text("S: Move Down");
            ImGui::Text("A: Move Left");
            ImGui::Text("D: Move Right");
            ImGui::Text("R: Reset Game");
            ImGui::Text("Camera: Auto-follow behind ship");
            
            ImGui::Separator();
            ImGui::Text("=== Game State ===");
            ImGui::Text("State: %s", 
                currentState == GameStateEnum::NEW_GAME ? "NEW_GAME" :
                currentState == GameStateEnum::PLAY_GAME ? "PLAY_GAME" : "END_GAME");
            ImGui::Text("Time: %.2f", gameState.play_time);
            ImGui::Text("Score: %d | Combo: %d", gameState.score, gameState.combo);
            ImGui::Text("Next Ring: %zu / %zu", gameState.next_ring_idx, rings.size());
            
            if (currentState == GameStateEnum::END_GAME) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "GAME OVER!");
                ImGui::Text("Press R to restart");
            }
            
            ImGui::Separator();
            ImGui::Text("=== Path Info ===");
            if (pathGenerator.is_valid()) {
                ImGui::Text("Path Length: %.2f", pathGenerator.get_total_length());
                ImGui::Text("Control Points: %zu", pathGenerator.get_control_points().size());
            } else {
                ImGui::Text("Path: Invalid");
            }
            
            ImGui::Separator();
            ImGui::Text("=== Ring Info ===");
            ImGui::Text("Total Rings: %zu", rings.size());
            if (!rings.empty()) {
                int passedCount = 0;
                int scoredCount = 0;
                for (const auto& ring : rings) {
                    if (ring.scored) scoredCount++;
                }
                ImGui::Text("Scored: %d", scoredCount);
            }
        }
        ImGui::End();
    }
    
    // Controls panel
    if (showControls) {
        bool opened = ImGui::Begin("Torus Ride Controls", &showControls);
        if (opened) {
            ImGui::Text("=== Game State ===");
            ImGui::Text("Current State: %s", 
                currentState == GameStateEnum::NEW_GAME ? "NEW_GAME" :
                currentState == GameStateEnum::PLAY_GAME ? "PLAY_GAME" : "END_GAME");
            if (currentState == GameStateEnum::END_GAME) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "GAME OVER! Press R to restart");
            }
            
            ImGui::Separator();
            ImGui::Text("=== Game Parameters ===");
            ImGui::SliderFloat("Ring Spacing", &gameParams.ring_spacing, 5.0f, 20.0f);
            ImGui::SliderFloat("Ring Radius (R)", &gameParams.ring_R, 1.0f, 5.0f);
            ImGui::SliderFloat("Ring Thickness (r)", &gameParams.ring_r, 0.1f, 1.0f);
            ImGui::SliderFloat("Pass Band", &gameParams.pass_band, 0.5f, 3.0f);
            ImGui::SliderFloat("Assist Strength", &gameParams.assist_strength, 0.0f, 1.0f);
            
            ImGui::Separator();
            ImGui::Text("=== Ship Info ===");
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", ship.position.x, ship.position.y, ship.position.z);
            ImGui::Text("Forward: (%.2f, %.2f, %.2f)", ship.forward.x, ship.forward.y, ship.forward.z);
            ImGui::Text("Speed: %.2f", ship.speed_current);
            ImGui::Text("Roll: %.2f deg", glm::degrees(ship.roll_angle));
            ImGui::Text("Pitch: %.2f deg", glm::degrees(ship.pitch_angle));
            ImGui::Text("Yaw: %.2f deg", glm::degrees(ship.yaw_angle));
            ImGui::SliderFloat("Ship Speed", &ship.speed_current, ship.speed_min, ship.speed_max);
            ImGui::SliderFloat("Speed Min", &ship.speed_min, 1.0f, 5.0f);
            ImGui::SliderFloat("Speed Max", &ship.speed_max, 5.0f, 15.0f);
            ImGui::SliderFloat("Ship Radius", &ship.radius, 0.1f, 1.0f);
            
            if (ImGui::Button("Reset Ship Position")) {
                ship.position = glm::vec3(0.0f);
                ship.forward = glm::vec3(0.0f, 0.0f, 1.0f);
            }
            
            if (ImGui::Button("Regenerate Rings")) {
                initializeTestRings();
            }
            
            ImGui::Separator();
            ImGui::Text("=== Path Controls ===");
            if (ImGui::Button("Generate Curved Path")) {
                initializeTestRings();
            }
            if (ImGui::Button("Generate Simple Line")) {
                generateSimpleLineRings();
            }
        }
        ImGui::End();
    }
}

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr), game(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5 - Torus Ride", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
	
	// Create the game instance
	game = std::make_unique<TorusRideGame>(mWindowManager, inputHandler, mCamera);
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

void
edaf80::Assignment5::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h

	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	auto lastTime = std::chrono::high_resolution_clock::now();

	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;

		auto& io = ImGui::GetIO();
		
		// Handle critical input BEFORE setting UI capture
		// Handle hot reload - Changed to F5 to avoid conflict with game reset
		if (inputHandler.GetKeycodeState(GLFW_KEY_F5) & JUST_PRESSED) {
			if (game) {
				// shader_reload_failed = !game->programManager.ReloadAllPrograms(); // Commented out for now
				if (shader_reload_failed) {
					tinyfd_notifyPopup("Shader Program Reload Error",
					                   "An error occurred while reloading shader programs; see the logs for details.\n"
					                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
					                   "error");
				} else {
					LogInfo("Shaders reloaded successfully");
				}
			}
		}
		
		// Set UI capture after handling critical input
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);
		
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);

		// Retrieve the actual framebuffer size
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);

		// Update game
		if (game) {
			game->update(deltaTimeUs);
		}

		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		// Render game
		if (!shader_reload_failed && game) {
			game->render(mCamera.GetWorldToClipMatrix());
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Render game UI
		if (game) {
			game->renderUI();
		}

		// Scene controls (legacy)
		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
