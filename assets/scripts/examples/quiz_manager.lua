-- quiz_manager.lua
-- Simple multiple-choice quiz controller.
--
-- Scene setup expected:
--   "QuizQuestion"  - TextComponent showing the current question
--   "QuizOptA"      - TextComponent for option A
--   "QuizOptB"      - TextComponent for option B
--   "QuizOptC"      - TextComponent for option C
--   "QuizFeedback"  - TextComponent showing "Correct!" / "Wrong!"
--   "QuizScore"     - TextComponent showing current score
--
-- Controls:
--   1 -> select option A
--   2 -> select option B
--   3 -> select option C

-- ---------------------------------------------------------------------------
-- Question bank  { question, {A, B, C}, correct_index (1-based) }
-- ---------------------------------------------------------------------------
local questions = {
    { "What colour is the sky?",
      {"Blue", "Green", "Red"}, 1 },
    { "How many sides does a triangle have?",
      {"4", "3", "5"}, 2 },
    { "What is 7 x 8?",
      {"54", "56", "64"}, 2 },
    { "Which planet is closest to the Sun?",
      {"Venus", "Earth", "Mercury"}, 3 },
    { "What is the boiling point of water (C)?",
      {"90", "120", "100"}, 3 },
}

local current   = 1
local score     = 0
local answered  = false
local feedTimer = 0.0
local FEED_TIME = 1.5   -- seconds to show feedback

local function ShowQuestion()
    local q = questions[current]
    Scene.SetText("QuizQuestion", q[1])
    Scene.SetText("QuizOptA", "1: " .. q[2][1])
    Scene.SetText("QuizOptB", "2: " .. q[2][2])
    Scene.SetText("QuizOptC", "3: " .. q[2][3])
    Scene.SetText("QuizFeedback", "")
    answered = false
end

local function Answer(choice)
    if answered then return end
    answered  = true
    feedTimer = FEED_TIME

    local q = questions[current]
    if choice == q[3] then
        score = score + 1
        Scene.SetText("QuizFeedback", "Correct!")
        Scene.SetColor("QuizFeedback", 0.2, 1.0, 0.2, 1.0)
    else
        Scene.SetText("QuizFeedback", "Wrong! Answer: " .. q[2][q[3]])
        Scene.SetColor("QuizFeedback", 1.0, 0.3, 0.3, 1.0)
    end

    Scene.SetText("QuizScore", "Score: " .. score .. "/" .. #questions)
end

function OnStart()
    Scene.SetText("QuizScore", "Score: 0/" .. #questions)
    ShowQuestion()
    Log("Quiz started! Use 1/2/3 to answer.")
end

function OnUpdate(dt)
    if feedTimer > 0.0 then
        feedTimer = feedTimer - dt
        if feedTimer <= 0.0 then
            current = current + 1
            if current > #questions then
                -- Quiz finished
                Scene.SetText("QuizQuestion", "Quiz complete!")
                Scene.SetText("QuizOptA", "")
                Scene.SetText("QuizOptB", "")
                Scene.SetText("QuizOptC", "")
                Scene.SetText("QuizFeedback",
                    "Final score: " .. score .. "/" .. #questions)
                Scene.SetColor("QuizFeedback", 1.0, 1.0, 1.0, 1.0)
            else
                ShowQuestion()
            end
        end
        return   -- block input while feedback is visible
    end

    if current > #questions then return end

    if Input.IsKeyPressed("1") then Answer(1) end
    if Input.IsKeyPressed("2") then Answer(2) end
    if Input.IsKeyPressed("3") then Answer(3) end
end
