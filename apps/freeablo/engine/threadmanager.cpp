#include "threadmanager.h"
#include "../farender/renderer.h"
#include <chrono>
#include <input/inputmanager.h>
#include <iostream>

namespace Engine
{
    ThreadManager* ThreadManager::mThreadManager = nullptr;
    ThreadManager* ThreadManager::get() { return mThreadManager; }

    ThreadManager::ThreadManager() : mQueue(100), mRenderState(nullptr), mAudioManager(50, 100) { mThreadManager = this; }

    void ThreadManager::run()
    {
        const int MAXIMUM_DURATION_IN_MS = 1000;
        Input::InputManager* inputManager = Input::InputManager::get();
        FARender::Renderer* renderer = FARender::Renderer::get();

        auto last = std::chrono::system_clock::now();
        size_t numFrames = 0;

        while (true)
        {
            while (mQueue.front())
            {
                handleMessage(*mQueue.front());
                mQueue.pop();
            }

            inputManager->poll();

            if (!renderer->renderFrame(mRenderState))
                break;

            auto now = std::chrono::system_clock::now();
            if (mRenderState)
                numFrames++;

            size_t duration =
                static_cast<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch() - last.time_since_epoch()).count());

            if (duration >= MAXIMUM_DURATION_IN_MS)
            {
                std::stringstream ss;
                ss << "(" << ((float)numFrames) / (((float)duration) / MAXIMUM_DURATION_IN_MS) << " FPS)";
                Render::setWindowTitle(Render::getWindowTitle() + " " + ss.str());
                numFrames = 0;
                last = now;
            }
        }
    }

    void ThreadManager::playMusic(const std::string& path)
    {
        Message message = {};
        message.type = ThreadState::PLAY_MUSIC;
        message.data.musicPath = new std::string(path);

        mQueue.push(message);
    }

    void ThreadManager::playSound(const std::string& path)
    {
        if (path.empty())
        {
            std::cerr << "Attempt to play invalid sound!" << std::endl;
            return;
        }

        Message message = {};
        message.type = ThreadState::PLAY_SOUND;
        message.data.soundPath = new std::string(path);

        mQueue.push(message);
    }

    void ThreadManager::stopSound()
    {
        Message message = {};
        message.type = ThreadState::STOP_SOUND;
        mQueue.push(message);
    }

    bool ThreadManager::isPlayingSound() const { return mAudioManager.isPlayingSound(); }

    void ThreadManager::sendRenderState(FARender::RenderState* state)
    {
        Message message = {};
        message.type = ThreadState::RENDER_STATE;
        message.data.renderState = state;

        mQueue.push(message);
    }

    void ThreadManager::handleMessage(const Message& message)
    {
        switch (message.type)
        {
            case ThreadState::PLAY_MUSIC:
            {
                mAudioManager.playMusic(*message.data.musicPath);
                delete message.data.musicPath;
                break;
            }

            case ThreadState::PLAY_SOUND:
            {
                mAudioManager.playSound(*message.data.soundPath);
                delete message.data.soundPath;
                break;
            }

            case ThreadState::STOP_SOUND:
            {
                mAudioManager.stopSound();
                break;
            }

            case ThreadState::RENDER_STATE:
            {
                if (mRenderState && mRenderState != message.data.renderState)
                    mRenderState->ready = true;

                mRenderState = message.data.renderState;
                break;
            }
        }
    }
}
