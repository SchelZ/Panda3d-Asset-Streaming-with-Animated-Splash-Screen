#include "prc_config.h"

#include "pandaFramework.h"
#include "pandaSystem.h"
#include "loader.h"
#include "loaderOptions.h"
#include "asyncTask.h"
#include "textNode.h"
#include "modelLoadRequest.h"
#include "asyncTaskManager.h"
#include "genericAsyncTask.h"

#include <numbers>
#include <cmath>
#include <format>
#include <iostream>

constexpr float PULSE_FREQ = 5.0f;

struct GameState {
    PT(ModelLoadRequest) load_req;
    NodePath splash_node;
    NodePath model;
    bool is_loaded = false;
} state;

AsyncTask::DoneStatus splash_task(GenericAsyncTask* task, void* data) {
    if (!state.is_loaded) {
        const float t = static_cast<float>(task->get_elapsed_time());
        const float s = 0.1f + (std::sin(t * PULSE_FREQ) * 0.02f);
        state.splash_node.set_scale(s);
        return AsyncTask::DS_cont;
    }
    state.splash_node.remove_node();
    return AsyncTask::DS_done;
}

AsyncTask::DoneStatus check_load_status(GenericAsyncTask* task, void* data) {
    if (!state.load_req->is_ready())
        return AsyncTask::DS_cont;

    PandaNode* node_ptr = state.load_req->get_model();
    if (node_ptr != nullptr) {
        WindowFramework* window = static_cast<WindowFramework*>(data);
        state.model = NodePath(node_ptr);
        state.model.reparent_to(window->get_render());
        state.model.set_pos(0.0f, 40.0f, 0.0f);
        std::cout << "Model loaded successfully!" << std::endl;
        state.is_loaded = true;
        return AsyncTask::DS_done;
    } else {
        std::cerr << "Error: Background loader returned a null model!" << std::endl;
        return AsyncTask::DS_cont;
    }
}

int main(int argc, char* argv[]) {
    load_embedded_prc();

    PandaFramework framework;
    framework.open_framework(argc, argv);
    WindowFramework* window = framework.open_window();
    window->enable_keyboard();

    PT(TextNode) text_obj = new TextNode("loading_ui");
    text_obj->set_text("LOADING...");
    text_obj->set_align(TextNode::A_center);
    text_obj->set_text_color(1.0f, 1.0f, 1.0f, 1.0f);

    state.splash_node = window->get_aspect_2d().attach_new_node(text_obj);
    state.splash_node.set_scale(0.1f);

    Filename model_path("panda.egg.pz");
    state.load_req = new ModelLoadRequest("panda_model", model_path, LoaderOptions(), Loader::get_global_ptr());
    Loader::get_global_ptr()->load_async(state.load_req);

    AsyncTaskManager* task_mgr = AsyncTaskManager::get_global_ptr();
    task_mgr->add(new GenericAsyncTask("SplashAnim", &splash_task, nullptr));
    task_mgr->add(new GenericAsyncTask("LoadMonitor", &check_load_status, window));

    framework.main_loop();
    framework.close_framework();
    return 0;
}
