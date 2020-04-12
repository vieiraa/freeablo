#include "quest.h"

#include "../itemfactory.h"
#include "../player.h"
#include "../world.h"
#include "questmanager.h"
#include <fmt/format.h>
#include <script/luascript.h>

namespace luabridge
{
    template <typename T> struct EnumWrapper
    {
        static inline typename std::enable_if<std::is_enum_v<T>, void>::type push(lua_State* L, T value)
        {
            lua_pushinteger(L, static_cast<std::size_t>(value));
        }

        static inline typename std::enable_if<std::is_enum_v<T>, T>::type get(lua_State* L, int index) { return static_cast<T>(lua_tointeger(L, index)); }
    };

    template <> struct Stack<FAWorld::Quest::QuestStatus> : EnumWrapper<FAWorld::Quest::QuestStatus>
    {
    };
}

namespace Script
{
    using namespace FAWorld;

    template <> void LuaScript::registerType<Quest>()
    {
        luabridge::getGlobalNamespace(mState)
            .beginClass<Quest>("Quest")
            .addConstructor<void (*)(const std::string&, const std::string&)>()
            .addProperty("status", &Quest::getStatus, &Quest::setStatus)
            .endClass();
    }
}

namespace FAWorld
{
    void Quest::trigger()
    {
        auto script = Script::LuaScript::getInstance();
        auto trigger = script->get(mId)["trigger"];
        trigger(this);
    }

    void Quest::rewardPlayer()
    {
        auto script = Script::LuaScript::getInstance();
        auto rewards = script->get(mId)["rewards"];
        rewards(this);
    }

    Quest Quest::loadFromScript(const std::string& scriptPath, const std::string& questId, World* world)
    {
        auto script = Script::LuaScript::getInstance();
        script->runScript(scriptPath);
        Quest ret = script->get(questId)["newQuest"]().cast<Quest>();
        ret.mWorld = world;
        return ret;
    }
}
