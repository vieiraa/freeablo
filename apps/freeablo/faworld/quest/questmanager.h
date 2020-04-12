#pragma once

#include <map>
#include <memory>

#include "quest.h"

namespace FAWorld
{
    class World;

    class QuestManager
    {
        std::map<std::string, Quest> mQuests;
        World& mWorld;

    public:
        QuestManager(World& world);

        void insertQuest(const std::string& id, Quest& quest);

        World& getWorld() { return mWorld; }
    };
}
