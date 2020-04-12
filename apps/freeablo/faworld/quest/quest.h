#pragma once

#include <functional>
#include <script/luascript.h>
#include <string>
#include <vector>

namespace FAWorld
{
    class Actor;
    class Item;
    class Player;
    class World;

    class Quest
    {
    public:
        union QuestGiver
        {
            Actor* npc;
            Item* book;
        };

        enum class QuestStatus : uint32_t
        {
            inactive = 0,
            available,
            active,
            finished
        };

        //private:
        std::string mId;
        std::string mName;
        QuestStatus mStatus;
        QuestGiver mGiver;
        World* mWorld;

        friend class QuestManager;

    public:
        Quest() = default;
        Quest(const std::string& id, const std::string& name) : mId(id), mName(name) {}
        Quest(const std::string& id, const std::string& name, QuestGiver giver) : mId(id), mName(name), mStatus(QuestStatus::inactive), mGiver(giver) {};

        void setStatus(QuestStatus status) { mStatus = status; }
        QuestStatus getStatus() const { return mStatus; }

        const std::string& getName() const { return mName; }

        void trigger();
        void rewardPlayer();
        void setGiverNpc(Actor* npc) { mGiver.npc = npc; }
        void setGiverBook(Item* book) { mGiver.book = book; }

        static Quest loadFromScript(const std::string& scriptPath, const std::string& questId, World* world);

    private:
        void setWorld(World* world) { mWorld = world; }
    };
}
