#include "game.h"


typedef std::tuple<GLboolean, Direction, glm::vec2> Collision;

SpriteRenderer* Renderer;
GameObject* Player;
BallObject* Ball;
ParticleGenerator* Particles;
PostProcessor* Effects;
irrklang::ISoundEngine* SoundEngine=irrklang::createIrrKlangDevice();
TextRenderer* Text;

// 初始化挡板的大小
const glm::vec2 PLAYER_SIZE(100, 20);
// 初始化挡板的速率
const GLfloat PLAYER_VELOCITY(500.0f);
// 初始化球的速度
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
// 球的半径
const GLfloat BALL_RADIUS = 12.5f;
// shake时间
GLfloat ShakeTime = 0.0f;


// function declaration
Direction VectorDirection(glm::vec2 target);
GLboolean CheckCollision(GameObject& one, GameObject& two);
Collision CheckCollision(BallObject& one, GameObject& two);
GLboolean ShouldSpawn(GLuint chance);
void ActivatePowerUp(PowerUp& powerUp);
GLboolean IsOtherPowerUpActive(std::vector<PowerUp>& powerUps, std::string type);


Game::Game(GLuint width, GLuint height)
    : State(GAME_START), Keys(), Width(width), Height(height) { }

Game::~Game() { }

void Game::Init()
{
    // 加载着色器
    ResourceManager::LoadShader("src/sprite.vs", "src/sprite.fs", nullptr, "sprite");
    ResourceManager::LoadShader("src/particle.vs", "src/particle.fs", nullptr, "particle");
    ResourceManager::LoadShader("src/post_processing.vs", "src/post_processing.fs", nullptr, "postprocessing");
    // 配置着色器
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->Width), 
        static_cast<GLfloat>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").Use().SetMatrix4("projection", projection);
    // 设置专用于渲染的控制
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    // 加载纹理
    ResourceManager::LoadTexture("resources/textures/awesomeface.png", GL_TRUE, "face");
    ResourceManager::LoadTexture("resources/textures/background.jpg", GL_FALSE, "background");
    ResourceManager::LoadTexture("resources/textures/block.png", GL_TRUE, "block");
    ResourceManager::LoadTexture("resources/textures/block_solid.png", GL_TRUE, "block_solid");
    ResourceManager::LoadTexture("resources/textures/paddle.png", GL_TRUE, "paddle");
    ResourceManager::LoadTexture("resources/textures/particle.png", GL_TRUE, "particle");
    ResourceManager::LoadTexture("resources/textures/powerup_speed.png", GL_TRUE, "powerup_speed");
    ResourceManager::LoadTexture("resources/textures/powerup_sticky.png", GL_TRUE, "powerup_sticky");
    ResourceManager::LoadTexture("resources/textures/powerup_increase.png", GL_TRUE, "powerup_increase");
    ResourceManager::LoadTexture("resources/textures/powerup_passthrough.png", GL_TRUE, "powerup_passthrough");
    ResourceManager::LoadTexture("resources/textures/powerup_chaos.png", GL_TRUE, "powerup_chaos");
    ResourceManager::LoadTexture("resources/textures/powerup_confuse.png", GL_TRUE, "powerup_confuse");
    // 加载关卡
    GameLevel one, two, three, four;
    //GameLevel test;
    one.Load("resources/levels/one.lvl", this->Width, this->Height * 0.5f);
    two.Load("resources/levels/two.lvl", this->Width, this->Height * 0.5f);
    three.Load("resources/levels/three.lvl", this->Width, this->Height * 0.5f);
    four.Load("resources/levels/four.lvl", this->Width, this->Height * 0.5f);
    //test.Load("resources/levels/zero.lvl", this->Width, this->Height * 0.5f);
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    //this->Levels.push_back(test);
    this->Level = 0;
    // 加载挡板
    glm::vec2 playerPos = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
    // 加载球
    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));
    // 加载粒子
    Particles = new ParticleGenerator(
        ResourceManager::GetShader("particle"), 
        ResourceManager::GetTexture("particle"), 
        1500
    );
    // 加载后处理
    Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
    // 加载声音
    SoundEngine->play2D("resources/audio/breakout.mp3", true);
    // 加载字
    Text = new TextRenderer(this->Width, this->Height);
    Text->Load("resources/fonts/ocraext.TTF", 24);

    this->Lives = 3;
    Effects->Chaos = true;
}

void Game::ResetLevel()
{
    if (this->Level == 0)this->Levels[0].Load("resources/levels/one.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 1)this->Levels[1].Load("resources/levels/two.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 2)this->Levels[2].Load("resources/levels/three.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 3)this->Levels[3].Load("resources/levels/four.lvl", this->Width, this->Height * 0.5f);
    //else if (this->Level == 4)this->Levels[4].Load("resources/levels/zero.lvl", this->Width, this->Height * 0.5f);
    this->Lives = 3;
}

void Game::ResetPlayer()
{
    // reset player/ball stats
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2((this->Width - Player->Size.x) / 2, this->Height - Player->Size.y);
    Ball->Reset(Player->Position +
        glm::vec2(Player->Size.x / 2 - Ball->Radius, -Ball->Radius * 2), INITIAL_BALL_VELOCITY);
    // also disable all active powerups
    Effects->Chaos = Effects->Confuse = false;
    Ball->PassThrough = Ball->Sticky = false;
    Player->Color = glm::vec3(1.0f);
    Ball->Color = glm::vec3(1.0f);
}

void Game::Update(GLfloat dt)
{
    // update ball
    Ball->Move(dt, this->Width);
    // update collision
    this->DoCollisions();
    if (Ball->Position.y >= this->Height)
    {
        --this->Lives;
        if (this->Lives == 0)
        {
            this->ResetLevel();
            this->State = GAME_MENU;
        }
        this->ResetPlayer();
    }
    // update particles
    Particles->Update(dt, *Ball, 1, glm::vec2(BALL_RADIUS / 2));
    // update shake time
    if (ShakeTime > 0.0f)
    {
        ShakeTime -= dt;
        if (ShakeTime <= 0.0f)
            Effects->Shake = false;
    }
    // update powerup
    this->UpdatePowerUps(dt);
    // check win
    if (this->State == GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
    {
        this->ResetLevel();
        this->ResetPlayer();
        Effects->Chaos = true;
        this->State = GAME_WIN;
        SoundEngine->play2D("resources/audio/victory.wav", false);
    }
}

void Game::ProcessInput(GLfloat dt)
{
    if (this->State == GAME_ACTIVE)
    {
        GLfloat velocity = dt * PLAYER_VELOCITY;
        // 移动挡板
        if (this->Keys[GLFW_KEY_A])
        {
            if (Player->Position.x >= 0)
            {
                Player->Position.x -= velocity;
                if (Ball->Stuck)
                    Ball->Position.x -= velocity;
            }
        }
        if (this->Keys[GLFW_KEY_D])
        {
            if (Player->Position.x <= this->Width - Player->Size.x)
            {
                Player->Position.x += velocity;
                if (Ball->Stuck)
                    Ball->Position.x += velocity;
            }
        }
        // 释放球
        if (this->Keys[GLFW_KEY_SPACE])
            Ball->Stuck = false;
    }
    if (this->State == GAME_MENU)
    {
        if (this->Keys[GLFW_KEY_ENTER] && !this->KeyProcessed[GLFW_KEY_ENTER])
        {
            this->State = GAME_ACTIVE;
            this->KeyProcessed[GLFW_KEY_ENTER] = true;
        }
        if (this->Keys[GLFW_KEY_W] && !this->KeyProcessed[GLFW_KEY_W])
        {
            this->Level = (this->Level + 1) % this->Levels.size();
            this->KeyProcessed[GLFW_KEY_W] = true;
        }
            
        if (this->Keys[GLFW_KEY_S] && !this->KeyProcessed[GLFW_KEY_S])
        {
            this->Level > 0 ? --this->Level : this->Level = this->Levels.size() - 1;
            this->KeyProcessed[GLFW_KEY_S] = true;
        }   
    }
    if (this->State == GAME_WIN)
    {
        if (this->Keys[GLFW_KEY_ENTER])
        {
            this->KeyProcessed[GLFW_KEY_ENTER] = true;
            Effects->Chaos = false;
            this->State = GAME_MENU;
        }
    }
    if (this->State == GAME_START)
    {
        if (this->Keys[GLFW_KEY_ENTER])
        {
            this->KeyProcessed[GLFW_KEY_ENTER] = true;
            this->State = GAME_MENU;
            Effects->Chaos = false;
        }
    }
}

void Game::Render()
{
    if (this->State == GAME_ACTIVE || this->State == GAME_MENU || this->State==GAME_START)
    {
        Effects->BeginRender();
            // 绘制背景
            Renderer->DrawSprite(ResourceManager::GetTexture("background"), 
                glm::vec2(0, 0), glm::vec2(this->Width, this->Height), 0.0f);
            // 绘制关卡
            this->Levels[this->Level].Draw(*Renderer);
            // 绘制挡板
            Player->Draw(*Renderer);
            // 绘制道具
            for (PowerUp& powerup : this->PowerUps)
                if (!powerup.Destroyed)
                    powerup.Draw(*Renderer);
            // 绘制粒子
            Particles->Draw();
            // 绘制球
            Ball->Draw(*Renderer);
        Effects->EndRender();
        Effects->Render(glfwGetTime());
        // 绘制文字
        std::stringstream ss; ss << this->Lives;
        Text->RenderText("Lives:" + ss.str(), 5.0f, 5.0f, 1.0f);
    }
    if (this->State == GAME_MENU)
    {
        std::stringstream levelss; levelss << this->Level + 1;
        Text->RenderText("Press ENTER to start", 250.0f, this->Height / 2, 1.0f);
        Text->RenderText("Press W or S to select level", 245.0f, this->Height / 2 + 20.0f, 0.75f);
        Text->RenderText("Level:" + levelss.str(), 5.0f, this->Height - 20.0, 1.0f);
    }
    if (this->State == GAME_WIN)
    {
        Text->RenderText(
            "You WON!!!", 320.0, this->Height / 2 - 20.0, 1.0, glm::vec3(1.0, 1.0, 0.0)
        );
        Text->RenderText(
            "Press ENTER to retry or ESC to quit", 130.0, Height / 2, 1.0, glm::vec3(1.0, 1.0, 0.0)
        );
    }
    if (this->State == GAME_START)
    {
        Text->RenderText(
            "BREAKOUT", 210.0, Height * 2 / 5, 3.0, glm::vec3(1.0, 1.0, 0.0)
        );
        Text->RenderText(
            "Press ENTER to start or ESC to quit", 130.0, Height * 2 / 3, 1.0, glm::vec3(1.0, 1.0, 0.0)
        );
    }
}

void Game::DoCollisions()
{
    if (this->State == GAME_ACTIVE)
    {
        // 砖块碰撞
        for (GameObject& box : this->Levels[this->Level].Bricks)
        {
            if (!box.Destroyed)
            {
                Collision collision = CheckCollision(*Ball, box);
                if (std::get<0>(collision))
                {
                    // 如果砖块不是实心就销毁砖块
                    if (!box.IsSolid)
                    {
                        box.Destroyed = GL_TRUE;
                        this->SpawnPowerUps(box);
                        SoundEngine->play2D("resources/audio/bleep.mp3", false);
                    }
                    else
                    {   // 如果是实心的砖块则激活shake特效
                        ShakeTime = 0.05f;
                        Effects->Shake = true;
                        SoundEngine->play2D("resources/audio/solid.wav", false);
                    }
                    // 碰撞处理
                    Direction dir = std::get<1>(collision);
                    glm::vec2 diff_vector = std::get<2>(collision);
                    if (!(Ball->PassThrough && !box.IsSolid))
                    {
                        if (dir == LEFT || dir == RIGHT) // 水平方向碰撞
                        {
                            Ball->Velocity.x = -Ball->Velocity.x; // 反转水平速度
                            // 重定位
                            GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
                            if (dir == LEFT)
                                Ball->Position.x += penetration; // 将球右移
                            else
                                Ball->Position.x -= penetration; // 将球左移
                        }
                        else // 垂直方向碰撞
                        {
                            Ball->Velocity.y = -Ball->Velocity.y; // 反转垂直速度
                            // 重定位
                            GLfloat penetration = Ball->Radius - std::abs(diff_vector.y);
                            if (dir == UP)
                                Ball->Position.y -= penetration; // 将球上移
                            else
                                Ball->Position.y += penetration; // 将球下移
                        }
                    }
                }
            }
        }
        // 挡板碰撞
        Collision result = CheckCollision(*Ball, *Player);
        if (!Ball->Stuck && std::get<0>(result))
        {
            // 检查碰到了挡板的哪个位置，并根据碰到哪个位置来改变速度
            GLfloat centerBoard = Player->Position.x + Player->Size.x / 2;
            GLfloat distance = (Ball->Position.x + Ball->Radius) - centerBoard;
            GLfloat percentage = distance / (Player->Size.x / 2);
            // 依据结果移动，撞击点距离挡板的中心点越远，则水平方向的速度就会越大
            GLfloat strength = 2.0f;
            glm::vec2 oldVelocity = Ball->Velocity;
            Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
            Ball->Velocity.y = -std::abs(Ball->Velocity.y);
            Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);

            Ball->Stuck = Ball->Sticky;
            SoundEngine->play2D("resources/audio/bleep.wav", false);
        }
        // 道具碰撞
        for (PowerUp& powerup : this->PowerUps)
        {
            if (!powerup.Destroyed)
            {
                if (powerup.Position.y >= this->Height)
                    powerup.Destroyed = GL_TRUE;
                if (CheckCollision(*Player, powerup))
                {   // 道具与挡板接触，激活它！
                    ActivatePowerUp(powerup);
                    powerup.Destroyed = GL_TRUE;
                    powerup.Activated = GL_TRUE;
                    SoundEngine->play2D("resources/audio/powerup.wav", false);
                }
            }
        }
    }
}

Direction VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f,1.0f),  // 上
        glm::vec2(1.0f,0.0f),  // 右
        glm::vec2(0.0f,-1.0f), // 下
        glm::vec2(-1.0f,0.0f), // 左
    };
    GLfloat max = 0.0f;
    GLuint best_match = -1;
    glm::vec2 normal_target = glm::normalize(target);
    for (GLuint i = 0; i < 4; i++)
    {
        GLfloat dot_res = glm::dot(normal_target, compass[i]);
        if (dot_res > max)
        {
            max = dot_res;
            best_match = i;
        }
    }
    return (Direction)best_match;
}

GLboolean CheckCollision(GameObject& one, GameObject& two) // AABB - AABB collision
{
    // x方向碰撞
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x 
        && two.Position.x + two.Size.x >= one.Position.x;
    // y方向碰撞
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y 
        && two.Position.y + two.Size.y >= one.Position.y;
    // 两个轴都碰撞则判定碰撞
    return collisionX && collisionY;
}

Collision CheckCollision(BallObject& one, GameObject& two) // AABB - Circle collision
{
    // 获取圆心
    glm::vec2 center(one.Position + one.Radius);
    // 获取aabb中心
    glm::vec2 aabb_half_extends(two.Size.x / 2, two.Size.y / 2);
    glm::vec2 aabb_center(two.Position.x + aabb_half_extends.x, two.Position.y + aabb_half_extends.y);
    // 两中心的矢量差
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extends, aabb_half_extends);
    // AABB_center加上clamped这样就得到了碰撞箱上距离圆最近的点closest
    glm::vec2 closest = aabb_center + clamped;
    // 获得圆心center和最近点closest的矢量并判断是否 length <= radius
    difference = closest - center;
    if (glm::length(difference) <= one.Radius)
        return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
    else
        return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

void Game::SpawnPowerUps(GameObject& block)
{
    if (ShouldSpawn(45))
        this->PowerUps.push_back(
            PowerUp("speed", glm::vec3(0.5f, 0.5f, 0.5f), 0.0f, block.Position, 
                ResourceManager::GetTexture("powerup_speed"))
        );
    if (ShouldSpawn(30))
        this->PowerUps.push_back(
            PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, 
                ResourceManager::GetTexture("powerup_sticky"))
        );
    if (ShouldSpawn(30))
        this->PowerUps.push_back(
            PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, 
                ResourceManager::GetTexture("powerup_passthrough"))
        );
    if (ShouldSpawn(30))
        this->PowerUps.push_back(
            PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 0.0f, block.Position, 
                ResourceManager::GetTexture("powerup_increase"))
        );
    if (ShouldSpawn(15)) // 负面道具被更频繁地生成
        this->PowerUps.push_back(
            PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 15.0f, block.Position, 
                ResourceManager::GetTexture("powerup_confuse"))
        );
    if (ShouldSpawn(15))
        this->PowerUps.push_back(
            PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 15.0f, block.Position, 
                ResourceManager::GetTexture("powerup_chaos"))
        );
}

GLboolean ShouldSpawn(GLuint chance)
{
    GLuint random = rand() % chance;
    return random == 0;
}

void ActivatePowerUp(PowerUp& powerUp)
{
    // 根据道具类型发动道具
    if (powerUp.Type == "speed")
    {
        Ball->Velocity *= 1.2;
    }
    else if (powerUp.Type == "sticky")
    {
        Ball->Sticky = GL_TRUE;
        Player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
    }
    else if (powerUp.Type == "pass-through")
    {
        Ball->PassThrough = GL_TRUE;
        Ball->Color = glm::vec3(1.0f, 0.5f, 0.5f);
    }
    else if (powerUp.Type == "pad-size-increase")
    {
        Player->Size.x += 50;
    }
    else if (powerUp.Type == "confuse")
    {
        if (!Effects->Chaos)
            Effects->Confuse = GL_TRUE; // 只在chaos未激活时生效，chaos同理
    }
    else if (powerUp.Type == "chaos")
    {
        if (!Effects->Confuse)
            Effects->Chaos = GL_TRUE;
    }
}

void Game::UpdatePowerUps(GLfloat dt)
{
    for (PowerUp& powerup : this->PowerUps)
    {
        powerup.Position += powerup.Velocity * dt;
        if (powerup.Activated)
        {
            powerup.Duration -= dt;
            if (powerup.Duration <= 0.0f)
            {
                // 之后会将这个道具移除
                powerup.Activated = GL_FALSE;
                // 停用效果
                if (powerup.Type == "sticky")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "sticky"))
                    {   // 仅当没有其他sticky效果处于激活状态时重置，以下同理
                        Ball->Sticky = GL_FALSE;
                        Player->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerup.Type == "pass-through")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
                    {
                        Ball->PassThrough = GL_FALSE;
                        Ball->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerup.Type == "confuse")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
                    {
                        Effects->Confuse = GL_FALSE;
                    }
                }
                else if (powerup.Type == "chaos")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
                    {
                        Effects->Chaos = GL_FALSE;
                    }
                }
            }
        }
    }
    this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
        [](const PowerUp& powerup) {return powerup.Destroyed && !powerup.Activated; }), 
        this->PowerUps.end());
}

GLboolean IsOtherPowerUpActive(std::vector<PowerUp>& powerUps, std::string type)
{
    for (const PowerUp& powerUp : powerUps)
    {
        if (powerUp.Activated)
            if (powerUp.Type == type)
                return GL_TRUE;
    }
    return GL_FALSE;
}