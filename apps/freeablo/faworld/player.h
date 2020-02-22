#pragma once
#include "actor.h"
#include "monster.h"

namespace FAWorld
{
    class PlayerBehaviour;

    enum class PlayerClass
    {
        warrior = 0,
        rogue,
        sorcerer,
        none,
    };

    // note that this function features misspelling of sorcerer as sorceror
    // because it's written this way on character panel
    const char* toString(PlayerClass value);

    class Player : public Actor
    {
    public:
        static const std::string typeId;
        const std::string& getTypeId() override { return typeId; }

        Player(World& world);
        Player(World& world, const DiabloExe::CharacterStats& charStats);
        void initCommon();
        Player(World& world, FASaveGame::GameLoader& loader);
        void save(FASaveGame::GameSaver& saver) override;

        virtual ~Player();
        void updateSprites() override;
        bool dropItem(const Misc::Point& clickedPoint);

        virtual void update(bool noclip) override;

        PlayerBehaviour* getPlayerBehaviour() { return (PlayerBehaviour*)mBehaviour.get(); }
        void setPlayerClass(PlayerClass playerClass);

        PlayerClass getClass() const { return mPlayerClass; }
        virtual bool canCriticalHit() const override { return mPlayerClass == PlayerClass::warrior; }

        bool castSpell(SpellId spell, Misc::Point targetPoint) override;
        SpellId getActiveSpell() const { return mActiveSpell; }
        void setActiveSpell(SpellId spell) { mActiveSpell = spell; }
        void castActiveSpell(Misc::Point targetPoint);

        virtual void calculateStats(LiveActorStats& stats, const ActorStats& actorStats) const override;

        // This isn't serialised as it must be set before saving can occur.
        bool mPlayerInitialised = false;

    protected:
        virtual DamageType getMeleeDamageType() const override;

    private:
        void init(const DiabloExe::CharacterStats& charStats);
        bool canTalkTo(Actor* actor);
        void onEnemyKilled(Actor* enemy) override;
        void addExperience(Actor& enemy);
        void levelUp(int32_t newLevel);

        static void initialiseActorStats(ActorStats& stats, const DiabloExe::CharacterStats& from);

    private:
        struct CalculateStatsCacheKey
        {
            BaseStats baseStats;
            const GameLevel* gameLevel = nullptr;
            int32_t level = 0;
            int32_t inventoryChangedCallCount = -1;
        };
        mutable CalculateStatsCacheKey mLastStatsKey; // not serialised, only used to determine if we need to recalculate stats

        int32_t mInventoryChangedCallCount = 0; // not serialised, only used to determine if inventory changed since we last calculated stats
        PlayerClass mPlayerClass = PlayerClass::warrior;
        SpellId mActiveSpell = SpellId::firebolt;
    };
}
