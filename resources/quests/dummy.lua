dummy = {}

function dummy.newQuest()
   print("test new")
   ret = Quest("dummy", "Dummy")
   ret:setStatus(0)
   return ret
end

function dummy.rewards(quest)
   print("test rewards")
   quest:setStatus(3)
end

function dummy.trigger(quest)
   print("test trigger")
   quest:setStatus(1)
end
