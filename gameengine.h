#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "rasterengine.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>

struct Vec2 {
    float x;
    float y;

    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const { return Vec2(x / s, y / s); }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(float s) { x /= s; y /= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSquared() const { return x * x + y * y; }

    Vec2 normalized() const {
        float l = length();
        return (l > 0.0001f) ? Vec2(x / l, y / l) : Vec2(0, 0);
    }

    static float dot(const Vec2& a, const Vec2& b) {
        return a.x * b.x + a.y * b.y;
    }

    static float cross(const Vec2& a, const Vec2& b) {
        return a.x * b.y - a.y * b.x;
    }
};

struct Wheel {
    Vec2 pos;
    Vec2 vel;
    float angle;
    float angularVel;
    float radius;
    float suspensionOffset;
    bool onGround;
    Vec2 groundNormal;

    Wheel() : angle(0), angularVel(0), radius(14.0f), suspensionOffset(0), onGround(false) {}
};

struct Collectible {
    enum Type { Coin, FuelCan };
    Type type;
    Vec2 pos;
    int value;
    bool collected;
    float animTimer;

    Collectible(Type t, Vec2 p, int val = 1)
        : type(t), pos(p), value(val), collected(false), animTimer(0) {}
};

struct Particle {
    Vec2 pos;
    Vec2 vel;
    uint32_t color;
    float life;     // 1.0 down to 0
    float maxLife;
    int size;
};

struct FloatingText {
    std::string text;
    Vec2 pos;
    uint32_t color;
    float life;
    float maxLife;
};

class GameEngine {
public:
    GameEngine(int screenWidth = 640, int screenHeight = 360);
    ~GameEngine();

    void reset();
    void resize(int screenWidth, int screenHeight);
    void update(float dt);
    void render(RasterBuffer& buffer);

    // Keyboard and control inputs
    void setGas(bool active) { m_gasInput = active; }
    void setBrake(bool active) { m_brakeInput = active; }
    void togglePause() { m_isPaused = !m_isPaused; }
    bool isPaused() const { return m_isPaused; }
    bool isGameOver() const { return m_isGameOver; }
    std::string getGameOverReason() const { return m_gameOverReason; }

    int getScore() const { return m_score; }
    int getCoins() const { return m_coinsCollected; }
    int getDistance() const { return static_cast<int>(m_maxDistance); }
    float getFuel() const { return m_fuel; }
    float getSpeedKmh() const;

    // Camera
    Vec2 getCameraPos() const { return m_cameraPos; }

    // World terrain calculation
    static float getTerrainHeight(float worldX);
    static Vec2 getTerrainNormal(float worldX);

private:
    void initLevel();
    void updatePhysics(float dt);
    void updateCollectibles(float dt);
    void updateParticles(float dt);
    void spawnSmoke(const Vec2& pos, const Vec2& dir);
    void spawnDirt(const Vec2& pos, const Vec2& normal);
    void spawnSparkles(const Vec2& pos, uint32_t color);
    void addFloatingText(const std::string& text, const Vec2& pos, uint32_t color);

    // Rendering subroutines
    void renderSky(RasterBuffer& buffer);
    void renderMountains(RasterBuffer& buffer);
    void renderTerrain(RasterBuffer& buffer);
    void renderCollectibles(RasterBuffer& buffer);
    void renderParticles(RasterBuffer& buffer);
    void renderVehicle(RasterBuffer& buffer);
    void renderHUD(RasterBuffer& buffer);
    void renderOverlays(RasterBuffer& buffer);

    // Transforms between World and Screen coordinates
    Vec2 worldToScreen(const Vec2& worldPt) const;
    Vec2 screenToWorld(const Vec2& screenPt) const;

    int m_screenWidth;
    int m_screenHeight;

    // Vehicle physics state
    Vec2 m_chassisPos;
    Vec2 m_chassisVel;
    float m_chassisAngle;       // radians
    float m_chassisAngularVel;
    Wheel m_rearWheel;
    Wheel m_frontWheel;
    Vec2 m_driverHeadPos;
    Vec2 m_driverHeadVel;

    // Vehicle config
    float m_wheelbaseHalf;
    float m_suspensionRestLength;
    float m_suspensionStiffness;
    float m_suspensionDamping;
    float m_chassisMass;
    float m_wheelMass;
    float m_momentOfInertia;

    // Input state
    bool m_gasInput;
    bool m_brakeInput;

    // Gameplay state
    float m_fuel;
    int m_score;
    int m_coinsCollected;
    float m_maxDistance;
    bool m_isGameOver;
    bool m_isPaused;
    std::string m_gameOverReason;

    // Air-time and flips
    float m_airTime;
    float m_airRotation;
    float m_prevChassisAngle;

    // Camera
    Vec2 m_cameraPos;
    Vec2 m_cameraTarget;

    // Environment & objects
    std::vector<Collectible> m_collectibles;
    std::vector<Particle> m_particles;
    std::vector<FloatingText> m_floatingTexts;
    float m_cloudOffset;
    float m_gameTime;
};

#endif // GAMEENGINE_H
