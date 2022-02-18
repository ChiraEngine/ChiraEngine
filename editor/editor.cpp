#include <core/engine.h>
#include <input/inputManager.h>
#include <sound/oggFileSound.h>
#include <hook/discordRPC.h>
#ifdef CHIRA_BUILD_WITH_STEAMWORKS
#include <hook/steamAPI.h>
#endif
#include <resource/provider/filesystemResourceProvider.h>
#include <i18n/translationManager.h>
#include <entity/model/mesh.h>
#include <entity/physics/bulletRigidBody.h>
#include <entity/camera/editorCamera.h>
#include <entity/gui/settings.h>

using namespace chira;

int main() {
    Engine::preInit("settings_editor.json");
    Resource::addResourceProvider(new FilesystemResourceProvider{"editor"});
    TranslationManager::addTranslationFile("file://i18n/editor");
    TranslationManager::addUniversalFile("file://i18n/editor");

    Engine::getSettingsLoader()->setValue("engineGui", "discordIntegration", true, false, true);
    bool discordEnabled;
    Engine::getSettingsLoader()->getValue("engineGui", "discordIntegration", &discordEnabled);
    if (discordEnabled) {
        DiscordRPC::init(TR("editor.discord.application_id"));
        DiscordRPC::setLargeImage("main_logo");
        DiscordRPC::setState("https://discord.gg/ASgHFkX");
    }

#ifdef CHIRA_BUILD_WITH_STEAMWORKS
    Engine::getSettingsLoader()->setValue("engine", "steamworks", true, true, true);
    // Steam API docs say this is bad practice, I say I don't care
    SteamAPI::generateAppIDFile(1728950);
#endif

    InputManager::addCallback(InputKeyButton{Key::ESCAPE, InputKeyEventType::PRESSED, []{
        if(auto window = dynamic_cast<Window*>(Engine::getWindow()))
        window->shouldStopAfterThisFrame();
    }});
    InputManager::addCallback(InputKeyButton{Key::ONE, InputKeyEventType::PRESSED, []{
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }});
    InputManager::addCallback(InputKeyButton{Key::TWO, InputKeyEventType::PRESSED, []{
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }});
    InputManager::addCallback(InputKeyButton{Key::M, InputKeyEventType::PRESSED, []{
        Engine::getSoundManager()->getSound("helloWorld")->play();
    }});

    if (!glfwInit()) {
        Logger::log(LogType::ERROR, "GLFW", TR("error.glfw.undefined"));
        exit(EXIT_FAILURE);
    }
    glfwSetErrorCallback([](int error, const char* description) {
        Logger::log(LogType::ERROR, "GLFW", TRF("error.glfw.generic", error, description));
    });

    Engine::SystemTimer = glfwGetTime;

    Engine::init([]{
        if(auto window = dynamic_cast<Window*>(Engine::getWindow()))
        window->displaySplashScreen();
        },[]{

        auto staticTeapot = new BulletRigidBody{"file://physics/ground_static.json"};
        staticTeapot->translate(glm::vec3{3,0,-13});
        staticTeapot->addChild(new Mesh{"file://meshes/teapot.json"});
        Engine::getWindow()->addChild(staticTeapot);

        auto fallingTeapot = new BulletRigidBody{"file://physics/cube_dynamic.json"};
        fallingTeapot->translate(glm::vec3{0,15,-10});
        fallingTeapot->addChild(new Mesh{"file://meshes/teapot.json"});
        Engine::getWindow()->addChild(fallingTeapot);

        auto settingsUI = new Settings{};
        Engine::getWindow()->addChild(settingsUI);
        InputManager::addCallback(InputKeyButton{Key::O, InputKeyEventType::PRESSED, [settingsUI]{
            settingsUI->setVisible(!settingsUI->isVisible());
        }});

        auto camera = new EditorCamera{CameraProjectionMode::PERSPECTIVE};
        Engine::getWindow()->addChild(camera);
        Engine::getWindow()->setCamera(camera);
        EditorCamera::setupKeybinds();
        camera->translate(glm::vec3{0,0,15});

#ifdef CHIRA_BUILD_WITH_ANGELSCRIPT
        Engine::getAngelscriptProvider()->addScript("file://scripts/testScript.as");
#endif

        auto sound = new OGGFileSound();
        sound->init("file://sounds/helloWorldCutMono.ogg");
        Engine::getSoundManager()->addSound("helloWorld", sound);

        auto teapotMesh = Resource::getResource<MeshDataResource>("file://meshes/teapot.json");
        const auto teapotShader = teapotMesh->getMaterial()->getShader();
        teapotShader->use();
        teapotShader->setUniform("light.ambient", 0.1f, 0.1f, 0.1f);
        teapotShader->setUniform("light.diffuse", 1.0f, 1.0f, 1.0f);
        teapotShader->setUniform("light.specular", 1.0f, 1.0f, 1.0f);
        teapotShader->setUniform("light.position", 0.0f, 5.0f, 0.0f);

        Engine::getWindow()->setSkybox("file://materials/skybox/shanghai.json");
    },true);
    Engine::run([]{
            glfwPollEvents();
            for (auto& frame : Engine::windows) {
                if(auto window = dynamic_cast<Window*>(frame.get())){
                    for (auto &keybind: InputManager::getKeyButtonCallbacks()) {
                        if (glfwGetKey(window->window, static_cast<int>(keybind.getKey())) && keybind.getEventType() == InputKeyEventType::REPEAT)
                            keybind();
                    }
                    for (auto &keybind: InputManager::getMouseButtonCallbacks()) {
                        if (glfwGetMouseButton(window->window, static_cast<int>(keybind.getKey())) && keybind.getEventType() == InputKeyEventType::REPEAT)
                            keybind();
                    }
                }
            }
        },[]{
        glfwTerminate();
    });
}
