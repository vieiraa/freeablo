#pragma once
#include "../components/diabloexe/talkdata.h"
#include "../fagui/talkdialoguepopup.h"
#include "../farender/animationplayer.h"
#include "actor/statemachine.h"
#include "actoranimationmanager.h"
#include "actorstats.h"
#include "behaviour.h"
#include "faction.h"
#include "gamelevel.h"
#include "inventory.h"
#include "missile/missileenums.h"
#include "movementhandler.h"
#include "position.h"
#include "target.h"
#include "world.h"
#include <boost/format.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>
#include <misc/direction.h>
#include <misc/misc.h>
#include <unordered_map>

namespace Random
{
    class Rng;
}

namespace FASaveGame
{
    class Loader;
    class Saver;
}

namespace FAWorld
{
    class Behaviour;
    class World;

    class Actor
    {
    public:
        Actor(World& world, const std::string& walkAnimPath = "", const std::string& idleAnimPath = "", const std::string& dieAnimPath = "");
        Actor(World& world, const DiabloExe::Npc& npc, const DiabloExe::DiabloExe& exe);
        Actor(World& world, FASaveGame::GameLoader& loader);
        virtual ~Actor();

        virtual void save(FASaveGame::GameSaver& saver);
        virtual bool checkHit(Actor* enemy);

        static const std::string typeId;
        virtual const std::string& getTypeId() { return typeId; }

        void pickupItem(Target::ItemTarget target);

        void teleport(GameLevel* level, Position pos);
        virtual void updateSprites() {}
        GameLevel* getLevel();
        World* getWorld() const { return &mWorld; }

        virtual int32_t meleeDamageVs(const Actor* actor) const;
        void doMeleeHit(Actor* enemy);
        void doMeleeHit(const Misc::Point& point);
        void startMeleeAttack(Misc::Direction direction);

        std::string getDieWav() const;
        std::string getHitWav() const;

        bool canIAttack(Actor* actor);
        virtual void update(bool noclip);
        void takeDamage(int32_t amount);
        void heal();
        void restoreMana();
        void stopAndPointInDirection(Misc::Direction direction);

        virtual void die();
        bool isDead() const;
        bool isEnemy(Actor* other) const;

        const std::unordered_map<std::string, std::string>& getMenuTalkData() const { return mMenuTalkData; }
        std::unordered_map<std::string, DiabloExe::TalkData>& getGossipData() { return mGossipData; }
        const std::unordered_map<std::string, DiabloExe::TalkData>& getGossipData() const { return mGossipData; }
        std::unordered_map<std::string, DiabloExe::QuestTalkData>& getQuestTalkData() { return mQuestTalkData; }
        DiabloExe::TalkData& getBeforeDungeonTalkData() { return mBeforeDungeonTalkData; }
        const DiabloExe::TalkData& getBeforeDungeonTalkData() const { return mBeforeDungeonTalkData; }
        const std::string& getNpcId() const { return mNpcId; }
        const std::string& getName() const { return mName; }
        const ActorStats& getStats() const { return mStats; }
        const Position& getPos() const { return mMoveHandler.getCurrentPosition(); }
        int32_t getId() const { return mId; }
        bool hasTarget() const;

        bool canTalk() const { return mMenuTalkData.size() > 0; }
        bool canInteractWith(Actor* actor);
        void dealDamageToEnemy(Actor* enemy, uint32_t damage);

        virtual bool needsToRecalculateStats() const { return false; };
        virtual void calculateStats(LiveActorStats& stats, const ActorStats& actorStats) const;
        void recalculateStats() { mStats.recalculateStats(); }

        // public member variables
        MovementHandler mMoveHandler;
        Target mTarget;
        ActorAnimationManager mAnimation;
        bool isAttacking = false;
        bool mInvuln = false;
        CharacterInventory mInventory;
        boost::optional<Misc::Direction> mMeleeAttackRequestedDirection; // this is really stupid but I don't know how else to do it
        const std::vector<std::unique_ptr<Missile::Missile>>& getMissiles() const { return mMissiles; }

        // TODO: hack, this should eventually be removed.
        // Try not to use it unless you have no other choice with the current structure.
        bool mIsTowner = false;

    protected:
        void activateMissile(MissileId id, Misc::Point targetPoint);
        virtual void onEnemyKilled(Actor* enemy) { UNUSED_PARAM(enemy); };

        // protected member variables
        std::unique_ptr<StateMachine> mActorStateMachine;
        ActorStats mStats;
        std::string mSoundPath;
        std::unique_ptr<Behaviour> mBehaviour;
        Faction mFaction;
        std::string mName; ///< Name as it appears in-game
        int32_t mId = -1;
        std::unordered_map<std::string, std::string> mMenuTalkData;               ///< Lines of dialogue
        std::unordered_map<std::string, DiabloExe::TalkData> mGossipData;         ///< Gossip dialogues
        std::unordered_map<std::string, DiabloExe::QuestTalkData> mQuestTalkData; ///< Quest dialogues
        DiabloExe::TalkData mBeforeDungeonTalkData;
        bool mDeadLastTick = false;
        World& mWorld;
        std::vector<std::unique_ptr<Missile::Missile>> mMissiles;

        // TODO: this var is only used for dialog code, which branches on which npc is being spoken to.
        // Eventually, we should add a proper dialog specification system, and get rid of this.
        std::string mNpcId;
    };
}
