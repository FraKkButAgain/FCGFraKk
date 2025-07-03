#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


// -------------------------
// Configurações
// -------------------------
constexpr int WIN_W = 800;
constexpr int WIN_H = 600;

constexpr int AREA_X = 40;
constexpr int AREA_Y = 20;
constexpr int AREA_W = 480;
constexpr int AREA_H = 560;

float playerX     = AREA_X + AREA_W / 2.0f;
float playerY     = AREA_Y + AREA_H - 30.0f;
float playerRad   = 8.0f;
float speedPixels = 300.0f;

int playerLives = 3;

int currentFrame = 0;
float frameTimer = 0.0f;
bool isMovingLeft = false;
bool isMovingRight = false;

float gameTime = 0.0f;

float columnSpawnTimer    = 10.0f;  
const  float columnInterval = 10.0f;

float damageFlickerTimer = 0.0f;
bool tookDamage = false;

int   playerPower       = 10;    
const int POWER_MAX     = 100;

float baseCooldown      = 0.20f; 
float currentCooldown   = baseCooldown;


// -------------------------
// Estrutura do tiro
// -------------------------
struct Bullet {
    float x, y, r;
    float dx, dy;
};

std::vector<Bullet> bullets;
float bulletSpeed = 450.0f;
float bulletCooldown = 0.2f;
float bulletTimer = 0.0f;

enum class MoveDir { None, Left, Right };
MoveDir playerDir = MoveDir::None;

// -------------------------
void drawCircle(float cx, float cy, float r, int segments = 24)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * M_PI * i / segments;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}

void drawPlayArea()
{
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(AREA_X,             AREA_Y);
    glVertex2f(AREA_X + AREA_W,    AREA_Y);
    glVertex2f(AREA_X + AREA_W,    AREA_Y + AREA_H);
    glVertex2f(AREA_X,             AREA_Y + AREA_H);
    glEnd();
}

GLuint loadTexture(const char* path)
{
    int w, h, nrChannels;
    stbi_set_flip_vertically_on_load(true); // manter a imagem "normal"
    unsigned char* data = stbi_load(path, &w, &h, &nrChannels, 0);

    if (!data) {
        std::cerr << "Erro ao carregar sprite: " << path << std::endl;
        return 0;
    }

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(data);
    return texID;
}

void drawSpriteFrame(GLuint texID, int frameIndex, float x, float y, float halfSize)
{
    float spriteWidth = 128.0f;
    float spriteHeight = 128.0f;
    float sheetWidth = 640.0f;
    float sheetHeight = 128.0f;

    // Coordenadas de textura
    float u = (frameIndex * spriteWidth) / sheetWidth;
    float du = spriteWidth / sheetWidth;
    float v = 0.0f;
    float dv = spriteHeight / sheetHeight;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID);
    glColor3f(1.0f, 1.0f, 1.0f); // importante para não "tingir" a textura

    glBegin(GL_QUADS);
    glTexCoord2f(u,     v + dv); glVertex2f(x - halfSize, y - halfSize);
    glTexCoord2f(u + du, v + dv); glVertex2f(x + halfSize, y - halfSize);
    glTexCoord2f(u + du, v);      glVertex2f(x + halfSize, y + halfSize);
    glTexCoord2f(u,     v);      glVertex2f(x - halfSize, y + halfSize);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

// -------------------------
// INIMIGOS
// -------------------------


#include <cmath>

void normalize(float& x, float& y) {
    float len = std::sqrt(x * x + y * y);
    if (len != 0) {
        x /= len;
        y /= len;
    }
}

enum class EnemyType { Normal, Shotgun };

struct Enemy {
    float x, y;
    float speed;
    int   hp;
    float shootCooldown;
    EnemyType type = EnemyType::Normal;  
};

struct EnemyBullet {
    float x, y;
    float dx, dy; 
    float speed;
};

std::vector<Enemy> enemies;
std::vector<EnemyBullet> enemyBullets;
float enemySpawnTimer = 0.0f;
float enemySpawnInterval = 2.0f;

float shotgunSpawnTimer    = 10.0f;  
float shotgunSpawnInterval = 10.0f;




// -------------------------
void processInput(GLFWwindow* win, float dt)
{
    float v = speedPixels;
    if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
        v *= 0.5f;


    playerDir = MoveDir::None;
    isMovingLeft = isMovingRight = false;

    if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) {
        playerX -= v * dt;
        playerDir = MoveDir::Left;
        isMovingLeft = true;
    }
    if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        playerX += v * dt;
        playerDir = MoveDir::Right;
        isMovingRight = true;
    }
    if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS) playerY -= v * dt;
    if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) playerY += v * dt;

    float minX = AREA_X + playerRad;
    float maxX = AREA_X + AREA_W - playerRad;
    float minY = AREA_Y + playerRad;
    float maxY = AREA_Y + AREA_H - playerRad;

    playerX = std::clamp(playerX, minX, maxX);
    playerY = std::clamp(playerY, minY, maxY);
}

// -------------------------


// -------------------------
// sistema de poder
// -------------------------

int extraDiagonalPairs(int power) {
    if (power >= 75) return 3;    // Ângulos: ±15°, ±30°, ±45°
    if (power >= 50) return 2;    // Ângulos: ±15°, ±30°
    if (power >= 25) return 1;    // Ângulo:  ±15°
    return 0;                     // só tiro vertical
}

int bulletColumns(int power) {
    if (power >= 100) return 5;
    if (power >= 70)  return 4;
    if (power >= 40)  return 3;
    if (power >= 20)  return 2;
    return 1;
}

void updateCooldownByPower() {
    int tiers = playerPower / 10;              
    currentCooldown = baseCooldown * std::pow(0.9f, tiers);
}

void fireBullets(GLFWwindow* win) {
    static float cooldown = 0.0f;
    float now = glfwGetTime();

    float fireRate = std::max(0.05f, 0.3f - 0.02f * (playerPower / 10)); 

    if (cooldown > now)
        return;

    cooldown = now + fireRate;

    int columns = bulletColumns(playerPower);
    int pairs   = extraDiagonalPairs(playerPower);

    float spacing = 10.0f; 

    float centerOffset = (columns - 1) * spacing / 2.0f;
    for (int i = 0; i < columns; ++i) {
        float offset = (i * spacing) - centerOffset;
        bullets.push_back({ playerX + offset, playerY, 5.0f, 0.0f, -1.0f }); 
    }


    std::vector<float> angles;
    for (int i = 1; i <= pairs; ++i) {
        angles.push_back(15.0f * i);   // direita
        angles.push_back(-15.0f * i);  // esquerda
    }

    for (float angDeg : angles) {
    float rad = angDeg * M_PI / 180.0f;

    float dx =  std::sin(rad); 
    float dy = -std::cos(rad); 

    bullets.push_back({ playerX + dx * 10.0f,
                        playerY + dy * 10.0f,
                        5.0f, dx, dy });
}
}

// -------------------------
// o main
// -------------------------
int main()
{
    if (!glfwInit()) return -1;

    GLFWwindow* win = glfwCreateWindow(WIN_W, WIN_H, "Totalmente não touhou", nullptr, nullptr);
    if (!win) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint playerTexture = loadTexture("../assets/sprites/player_1.png");

    glViewport(0, 0, WIN_W, WIN_H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIN_W, WIN_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(win)) {

        // coisas de timer
        // pq eu realmente n tava com saco de criar um nivel
        float now = glfwGetTime();
        float dt  = now - lastTime;
        gameTime += dt;
        columnSpawnTimer -= dt;
        enemySpawnInterval = std::max(0.5f, 3.0f - gameTime * 0.01f);
        lastTime  = now;

        if (tookDamage) {
            damageFlickerTimer -= dt;
            if (damageFlickerTimer <= 0.0f) {
                tookDamage = false;
            }
        }

        bulletTimer -= dt;

        // jogo

        processInput(win, dt);

        if (glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS) {
            fireBullets(win);
        }
        enemySpawnTimer -= dt;
        shotgunSpawnTimer -= dt;

        if (enemySpawnTimer <= 0.0f) {
            int enemiesToSpawn = 1 + static_cast<int>(gameTime / 60.0f);
            for (int i = 0; i < enemiesToSpawn; ++i) {
                Enemy e;
                e.x = AREA_X + 20 + rand() % (AREA_W - 40);
                e.y = AREA_Y + 32.0f;
                e.speed = 70.0f;
                e.hp = 5;
                e.shootCooldown = std::max(0.5f, 1.5f - gameTime * 0.01f);
                e.type = EnemyType::Normal;
                enemies.push_back(e);

                
            }
            enemySpawnTimer = enemySpawnInterval;
        }

        if (shotgunSpawnTimer <= 0.0f) {

            Enemy s;
            s.x = AREA_X + 20 + rand() % (AREA_W - 40);
            s.y = AREA_Y + 32.0f;          
            s.speed = 0.0f;               
            s.hp    = 25;
            s.shootCooldown = 1.0f;   
            s.type  = EnemyType::Shotgun;

            enemies.push_back(s);

            // intervalo decresce 2 % a cada spawn (mínimo 2 s)
            shotgunSpawnInterval = std::max(2.0f, shotgunSpawnInterval * 0.98f);
            shotgunSpawnTimer    = shotgunSpawnInterval;
        }

        if (columnSpawnTimer <= 0.0f) {
            int columnsThisSpawn = 1 + static_cast<int>(gameTime / 30.0f);
            for (int c = 0; c < columnsThisSpawn; ++c) {

                
                float baseX = AREA_X + 40 + rand() % (AREA_W - 80);
                float baseY = AREA_Y + 0.0f;   

                
                for (int i = 0; i < 5; ++i) {
                    Enemy e;
                    e.x = baseX;
                    e.y = baseY - i * 40.0f;
                    e.speed = 50.0f;                                     
                    e.hp = 2;                                            
                    e.shootCooldown = std::max(0.5f, 1.5f - gameTime * 0.01f);
                    e.type = EnemyType::Normal; 
                    enemies.push_back(e);
                }
            }


            columnSpawnTimer = columnInterval;
        }

        for (auto& e : enemies) {
        e.y += e.speed * dt;

        e.shootCooldown -= dt;
        if (e.shootCooldown <= 0.0f) {

            float dx = playerX - e.x;
            float dy = playerY - e.y;
            normalize(dx, dy);

            if (e.type == EnemyType::Normal) {                           // ⬅ inalterado
                enemyBullets.push_back({ e.x, e.y, dx, dy, 150.0f });
                e.shootCooldown = std::max(0.5f, 1.5f - gameTime * 0.01f);

            } else { // ───────────── Shotgun ─────────────
                // ângulo base para o jogador
                float baseAng = std::atan2(dy, dx);

                // offsets em graus: não inclui 0°  → balas “grazem” o player
                const float offDeg[6] = { -18.f, -12.f, -6.f, 6.f, 12.f, 18.f };
                for (float off : offDeg) {
                    float rad = baseAng + off * M_PI / 180.0f;
                    float sx  = std::cos(rad);
                    float sy  = std::sin(rad);
                    enemyBullets.push_back({ e.x, e.y, sx, sy, 200.0f }); // pouca ^ mais rápida
                }
                e.shootCooldown = 1.0f;  // 1 tiro por segundo
                }
            }
        }

        for (auto& b : enemyBullets) {
        b.x += b.dx * b.speed * dt;
        b.y += b.dy * b.speed * dt;
        }

        enemyBullets.erase(
        std::remove_if(enemyBullets.begin(), enemyBullets.end(),
            [](EnemyBullet& b) {
                return b.x < AREA_X || b.x > AREA_X + AREA_W || b.y < AREA_Y || b.y > AREA_Y + AREA_H;
            }),
        enemyBullets.end()
        );

        for (auto& b : bullets) {
            for (auto& e : enemies) {
                float dx = b.x - e.x;
                float dy = b.y - e.y;
                float dist2 = dx*dx + dy*dy;
                if (dist2 < 20.0f * 20.0f) { // colisão
                    e.hp--;
                    
                if (e.hp == 0) {
                    int powerGain = (e.type == EnemyType::Shotgun) ? 10 : 1;
                    playerPower = std::min(POWER_MAX, playerPower + powerGain);
                    updateCooldownByPower();
                }
                    b.y = -1000;
                }
            }
        }


        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(),
                [](Bullet& b) {
                    return b.y + b.r < AREA_Y || b.y - b.r > AREA_Y + AREA_H ||
                        b.x + b.r < AREA_X || b.x - b.r > AREA_X + AREA_W;
                }),
            bullets.end()
        );

        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                [](Enemy& e) { return e.hp <= 0 || e.y > AREA_Y + AREA_H; }),
            enemies.end()
        );

        

        bool hitPlayer = false;


        // Verifica colisão com tiros inimigos
        for (auto& b : enemyBullets) {
            float dx = playerX - b.x;
            float dy = playerY - b.y;
            float dist2 = dx * dx + dy * dy;
            if (dist2 < playerRad * playerRad) { // mesma hitbox do círculo
                hitPlayer = true;
                break;
            }
        }

        if (hitPlayer) {
            playerLives--;

            playerPower = playerPower / 2;   
            updateCooldownByPower();

            // limpa tudo na tela
            enemies.clear();
            bullets.clear();
            enemyBullets.clear();

            // jogador continua na mesma posição
            std::cout << "Jogador foi atingido! Vidas restantes: " << playerLives << "\n";

            // Ativa flicker de dano
            tookDamage = true;
            damageFlickerTimer = 2.0f; // piscar por 2 segundos

            std::cout << "Vidas: " << playerLives
            << " | Power: " << playerPower << "\n";
        }


        // ---------- Render ----------
        glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Área de jogo
        glColor3f(0.0f, 0.9f, 0.5f);
        drawPlayArea();

        int frame = 0;

        if (playerDir == MoveDir::Left) {
            int tick = static_cast<int>(glfwGetTime() * 10) % 2; 
            frame = tick + 1;                                   
        }
        else if (playerDir == MoveDir::Right) {
            int tick = static_cast<int>(glfwGetTime() * 10) % 2; 
            frame = tick + 3;                                    
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        if (playerDir == MoveDir::Left) {
            if (currentFrame != 1 && currentFrame != 2) {
                currentFrame = 1;
                frameTimer = 0.1f;
            }
            if (frameTimer > 0.0f) {
                frameTimer -= dt;
                if (frameTimer <= 0.0f)
                    currentFrame = 2;
            }
        }
        else if (playerDir == MoveDir::Right) {
            if (currentFrame != 3 && currentFrame != 4) {
                currentFrame = 3;
                frameTimer = 0.1f;
            }
            if (frameTimer > 0.0f) {
                frameTimer -= dt;
                if (frameTimer <= 0.0f)
                    currentFrame = 4;
            }
        }
        else {
            currentFrame = 0; // parado
        }
        // Renderiza o sprite
        glColor3f(1.0f, 1.0f, 1.0f);
        bool drawPlayer = true;

        if (tookDamage) {
            int flickerTick = static_cast<int>(glfwGetTime() * 10) % 2;
            drawPlayer = (flickerTick == 0); // alterna entre visível e invisível
        }

        if (drawPlayer) {
            glColor3f(1.0f, 1.0f, 1.0f);
            drawSpriteFrame(playerTexture, currentFrame, playerX, playerY, 32.0f);
        }


        // Jogador
        if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
            glColor3f(1.0f, 0.2f, 0.2f); 
            drawCircle(playerX, playerY, playerRad);
        }
        
        // ---------------------
        // Inimigos
        glColor3f(1.0f, 0.2f, 0.2f);
        for (auto& e : enemies) {
            if (e.type == EnemyType::Shotgun) 
                glColor3f(1.0f, 0.4f, 0.8f); // lilás
            else                             
                glColor3f(1.0f, 0.2f, 0.2f); 
            
            glBegin(GL_QUADS);
            glVertex2f(e.x - 16, e.y - 16);
            glVertex2f(e.x + 16, e.y - 16);
            glVertex2f(e.x + 16, e.y + 16);
            glVertex2f(e.x - 16, e.y + 16);
            glEnd();
            
        }

        // Tiros dos inimigos
        glColor3f(1.0f, 1.0f, 0.0f);
        for (auto& b : enemyBullets) {
            drawCircle(b.x, b.y, 10.0f);
        }

        // Atualiza os tiros
        for (auto& b : bullets) {
            b.x += b.dx * bulletSpeed * dt;
            b.y += b.dy * bulletSpeed * dt;
        }
        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(),
                [](Bullet& b) { return b.y + b.r < 0; }),
            bullets.end()
        );


        // Tiros
        glColor3f(1.0f, 1.0f, 0.2f);
        for (auto& b : bullets) {
            drawCircle(b.x, b.y, b.r);
        }

        std::cout << "Inimigos na tela: " << enemies.size() << "\r";

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
    }
    