#include "questmanager.h"
#include <misc/misc.h>
#include "../world.h"

namespace FAWorld
{
    QuestManager::QuestManager(World& world) : mQuests{}, mWorld{world}
    {
        Script::LuaScript::getInstance()->registerType<Quest>();

        Quest dummy = Quest::loadFromScript((Misc::getResourcesPath() / "quests/dummy.lua").str(), "dummy", &mWorld);
        insertQuest("dummy", dummy);
    }

    void QuestManager::insertQuest(const std::string& id, Quest& quest)
    {
        mQuests[id] = quest;
    }
}
