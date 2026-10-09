#include "gameengine.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Fast pseudorandom hash for deterministic terrain & decoration
static inline float hash1D(int n) {
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

GameEngine::GameEngine(int screenWidth, int screenHeight)
    : m_screenWidth(screenWidth)
    , m_screenHeight(screenHeight)
    , m_chassisAngle(0)
    , m_chassisAngularVel(0)
    , m_wheelbaseHalf(26.0f)
    , m_suspensionRestLength(14.0f)
    , m_suspensionStiffness(1800.0f)
    , m_suspensionDamping(80.0f)
    , m_chassisMass(25.0f)
    , m_wheelMass(4.0f)
    , m_momentOfInertia(1100.0f)
    , m_gasInput(false)
    , m_brakeInput(false)
    , m_fuel(100.0f)
    , m_score(0)
    , m_coinsCollected(0)
    , m_maxDistance(0)
    , m_isGameOver(false)
    , m_isPaused(false)
    , m_gameOverReason("")
    , m_airTime(0)
    , m_airRotation(0)
    , m_prevChassisAngle(0)
    , m_cloudOffset(0)
    , m_gameTime(0)
{
    reset();
}

GameEngine::~GameEngine() = default;

void GameEngine::resize(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
}

float GameEngine::getTerrainHeight(float worldX) {
    if (worldX <= 80.0f) {
        return 0.0f; // completely flat starting runway
    }

    float x = worldX - 80.0f;

    // Difficulty smoothly progresses as distance increases:
    // Stage 1 (0 to 600 units / ~0m to 60m): Easy Peasy (gentle rolling plains, max slope ~4 deg)
    // Stage 2 (600 to 2000 units / ~60m to 200m): Casual rolling hills & mini jumps (max slope ~12-16 deg)
    // Stage 3 (2000 to 4500 units / ~200m to 450m): Moderate climbs & valleys (max slope ~25-28 deg)
    // Stage 4 (4500+ units / ~450m+): Advanced mountain passes, steep peaks & ramps (max slope ~38-42 deg)

    float wEasy = std::clamp(x / 400.0f, 0.0f, 1.0f);
    float wCasual = std::clamp((x - 600.0f) / 600.0f, 0.0f, 1.0f);
    float wMedium = std::clamp((x - 1800.0f) / 800.0f, 0.0f, 1.0f);
    float wHard = std::clamp((x - 4000.0f) / 1000.0f, 0.0f, 1.0f);

    // Stage 1: Easy peasy gentle swells (amplitude ~10-14px, wavelength ~500px)
    float h1 = (12.0f * std::sin(x * 0.0035f) + 6.0f * std::sin(x * 0.008f)) * wEasy;

    // Stage 2: Casual rolling hills (amplitude ~25-35px, wavelength ~300px)
    float h2 = (28.0f * std::sin(x * 0.006f + 0.5f) + 14.0f * std::cos(x * 0.012f)) * wCasual;

    // Stage 3: Moderate climbs and fun roller dips
    float h3 = (45.0f * std::sin(x * 0.005f + 1.2f) + 24.0f * std::cos(x * 0.018f) + 12.0f * std::sin(x * 0.035f)) * wMedium;

    // Stage 4: Advanced mountain peaks and steep inclines
    float h4 = (65.0f * std::sin(x * 0.004f + 2.0f) + 38.0f * std::cos(x * 0.011f) + 20.0f * std::sin(x * 0.022f)) * wHard;

    // Occasional launch ramps in medium and hard stages
    float ramp = 0.0f;
    if (wMedium > 0.5f) {
        float rampPhase = std::fmod(x, 750.0f);
        if (rampPhase > 480.0f && rampPhase < 620.0f) {
            float rT = (rampPhase - 480.0f) / 140.0f;
            ramp = 35.0f * wMedium * std::sin(rT * static_cast<float>(M_PI));
        }
    }

    return h1 + h2 + h3 + h4 + ramp;
}

Vec2 GameEngine::getTerrainNormal(float worldX) {
    float dx = 2.0f;
    float y1 = getTerrainHeight(worldX - dx);
    float y2 = getTerrainHeight(worldX + dx);
    float slope = (y2 - y1) / (2.0f * dx);
    Vec2 n(-slope, 1.0f);
    return n.normalized();
}

void GameEngine::initLevel() {
    m_collectibles.clear();

    // Spawn coins and fuel canisters along the track
    float x = 200.0f;
    int coinGroupCount = 0;
    while (x < 15000.0f) {
        float h = getTerrainHeight(x);

        // Every 350-450 units, place a Fuel Can
        if (std::fmod(x, 420.0f) < 45.0f) {
            m_collectibles.emplace_back(Collectible::FuelCan, Vec2(x, h + 30.0f), 100);
            x += 80.0f;
            continue;
        }

        // Place groups of coins
        int groupSize = 3 + (coinGroupCount % 4);
        for (int i = 0; i < groupSize; ++i) {
            float cx = x + i * 22.0f;
            float cy = getTerrainHeight(cx) + 24.0f + 6.0f * std::sin(i * 0.7f);
            int val = (i == groupSize - 1) ? 25 : 5;
            m_collectibles.emplace_back(Collectible::Coin, Vec2(cx, cy), val);
        }
        coinGroupCount++;
        x += groupSize * 22.0f + 60.0f + (hash1D(coinGroupCount) + 1.0f) * 40.0f;
    }
}

void GameEngine::reset() {
    float startGroundY = getTerrainHeight(0.0f); // 0.0f
    float wheelR = 14.0f;
    float restingRideHeight = 14.0f; // nominal height of mount above ground

    m_chassisPos = Vec2(0.0f, startGroundY + wheelR + restingRideHeight);
    m_chassisVel = Vec2(0.0f, 0.0f);
    m_chassisAngle = 0.0f;
    m_chassisAngularVel = 0.0f;

    m_rearWheel.pos = m_chassisPos + Vec2(-m_wheelbaseHalf, -restingRideHeight);
    m_rearWheel.vel = Vec2(0.0f, 0.0f);
    m_rearWheel.angle = 0.0f;
    m_rearWheel.angularVel = 0.0f;
    m_rearWheel.suspensionOffset = 0.0f;
    m_rearWheel.onGround = true;

    m_frontWheel.pos = m_chassisPos + Vec2(m_wheelbaseHalf, -restingRideHeight);
    m_frontWheel.vel = Vec2(0.0f, 0.0f);
    m_frontWheel.angle = 0.0f;
    m_frontWheel.angularVel = 0.0f;
    m_frontWheel.suspensionOffset = 0.0f;
    m_frontWheel.onGround = true;

    m_driverHeadPos = m_chassisPos + Vec2(0.0f, 22.0f);
    m_driverHeadVel = Vec2(0.0f, 0.0f);

    m_gasInput = false;
    m_brakeInput = false;

    m_fuel = 100.0f;
    m_score = 0;
    m_coinsCollected = 0;
    m_maxDistance = 0.0f;
    m_isGameOver = false;
    m_isPaused = false;
    m_gameOverReason = "";

    m_airTime = 0.0f;
    m_airRotation = 0.0f;
    m_prevChassisAngle = 0.0f;

    m_cameraPos = m_chassisPos;
    m_cameraTarget = m_chassisPos;

    m_particles.clear();
    m_floatingTexts.clear();

    initLevel();
}

float GameEngine::getSpeedKmh() const {
    float speedWorld = m_chassisVel.length();
    return speedWorld * 0.28f; // realistic scale
}

void GameEngine::update(float dt) {
    if (m_isPaused) return;

    m_gameTime += dt;
    m_cloudOffset += dt * 8.0f;

    if (!m_isGameOver) {
        // Fuel consumption: Zero drain when not driving!
        float fuelDrain = 0.0f;
        if (m_gasInput) {
            fuelDrain = 2.0f; // ~50s of continuous full throttle
        } else if (m_brakeInput && m_chassisVel.length() > 2.0f) {
            fuelDrain = 0.6f; // slight drain only while actively reversing in motion
        }
        m_fuel -= fuelDrain * dt;

        if (m_fuel <= 0.0f) {
            m_fuel = 0.0f;
            // Car coasts until coming to a complete stop
            if (m_chassisVel.length() < 2.5f && (m_rearWheel.onGround || m_frontWheel.onGround)) {
                m_isGameOver = true;
                m_gameOverReason = "OUT OF FUEL!";
            }
        }

        // Distance & score tracking
        float currentDist = std::max(0.0f, m_chassisPos.x / 10.0f);
        if (currentDist > m_maxDistance) {
            m_score += static_cast<int>((currentDist - m_maxDistance) * 5.0f);
            m_maxDistance = currentDist;
        }

        // Air time & rotation calculation
        bool inAir = (!m_rearWheel.onGround && !m_frontWheel.onGround);
        if (inAir) {
            m_airTime += dt;
            float angleDelta = m_chassisAngle - m_prevChassisAngle;
            // Wrap angle delta
            while (angleDelta > static_cast<float>(M_PI)) angleDelta -= 2.0f * static_cast<float>(M_PI);
            while (angleDelta < -static_cast<float>(M_PI)) angleDelta += 2.0f * static_cast<float>(M_PI);
            m_airRotation += angleDelta;

            // Flip detection (full 360 degree rotation)
            if (m_airRotation > 2.0f * static_cast<float>(M_PI)) {
                m_airRotation -= 2.0f * static_cast<float>(M_PI);
                m_score += 500;
                addFloatingText("BACKFLIP! +500", m_chassisPos + Vec2(0, 40), 0xFFFFAA00);
            } else if (m_airRotation < -2.0f * static_cast<float>(M_PI)) {
                m_airRotation += 2.0f * static_cast<float>(M_PI);
                m_score += 500;
                addFloatingText("FRONTFLIP! +500", m_chassisPos + Vec2(0, 40), 0xFFFFAA00);
            }
        } else {
            if (m_airTime > 1.2f) {
                int airBonus = static_cast<int>(m_airTime * 150.0f);
                m_score += airBonus;
                addFloatingText("BIG AIR! +" + std::to_string(airBonus), m_chassisPos + Vec2(0, 40), 0xFF55FFFF);
            }
            m_airTime = 0.0f;
            m_airRotation = 0.0f;
        }
        m_prevChassisAngle = m_chassisAngle;

        updateCollectibles(dt);
    }

    // Fixed substep physics for rock solid stability
    int subSteps = 6;
    float subDt = dt / subSteps;
    for (int step = 0; step < subSteps; ++step) {
        updatePhysics(subDt);
    }

    updateParticles(dt);

    // Smooth camera follow
    Vec2 forwardLook(m_chassisVel.x * 0.4f + 70.0f, m_chassisVel.y * 0.2f);
    m_cameraTarget = m_chassisPos + forwardLook;
    m_cameraPos += (m_cameraTarget - m_cameraPos) * std::min(1.0f, dt * 5.0f);
}

void GameEngine::updatePhysics(float dt) {
    const Vec2 gravity(0.0f, -420.0f);
    float cosA = std::cos(m_chassisAngle);
    float sinA = std::sin(m_chassisAngle);

    auto rotate = [cosA, sinA](const Vec2& v) {
        return Vec2(v.x * cosA - v.y * sinA, v.x * sinA + v.y * cosA);
    };

    // Chassis local orientation axes
    Vec2 localUp(-sinA, cosA);
    Vec2 forwardDir(cosA, sinA);

    // Rigid body wheel mount arms (relative to chassis center of mass)
    Vec2 localRearMount(-m_wheelbaseHalf, -14.0f);
    Vec2 localFrontMount(m_wheelbaseHalf, -14.0f);

    Vec2 armRear = rotate(localRearMount);
    Vec2 armFront = rotate(localFrontMount);

    Vec2 rearMount = m_chassisPos + armRear;
    Vec2 frontMount = m_chassisPos + armFront;

    // Apply gravity to vehicle
    m_chassisVel += gravity * dt;

    bool rearOnGround = false;
    bool frontOnGround = false;

    // Ground contact evaluation for both wheels
    auto checkWheelGround = [&](Wheel& w, const Vec2& mount, bool& onGroundOut) {
        float wheelR = w.radius; // 14.0f
        float gh = getTerrainHeight(mount.x);
        Vec2 normal = getTerrainNormal(mount.x);
        Vec2 tangent(normal.y, -normal.x);

        float dist = mount.y - gh;
        float penetration = wheelR - dist;

        if (penetration > 0.0f) {
            w.onGround = true;
            onGroundOut = true;
            w.groundNormal = normal;

            // Subtle suspension compression: 0 to 3.0 pixels travel
            w.suspensionOffset = std::clamp(penetration * 0.25f, 0.0f, 3.0f);

            // Cancel downward velocity into terrain (low bounce for planted feel)
            float vNorm = Vec2::dot(m_chassisVel, normal);
            if (vNorm < 0.0f) {
                m_chassisVel -= normal * (vNorm * 1.05f);
            }

            // Smooth upward terrain support force
            float supportAccel = std::clamp(penetration * 90.0f, 0.0f, 260.0f);
            m_chassisVel += normal * (supportAccel * dt);

            // Gentle position correction
            m_chassisPos += normal * (penetration * 0.08f);

            // Traction & Drive along terrain tangent
            float vTang = Vec2::dot(m_chassisVel, tangent);
            if (m_fuel > 0.0f && !m_isGameOver) {
                if (m_gasInput) {
                    float a_drive = 135.0f; // fast, thrilling climb power
                    m_chassisVel += tangent * (a_drive * dt);
                    w.angularVel = std::max(vTang / wheelR, 42.0f);
                }
                if (m_brakeInput) {
                    float a_brake = (vTang > 2.0f) ? -170.0f : -70.0f;
                    m_chassisVel += tangent * (a_brake * dt);
                    w.angularVel *= (1.0f - 15.0f * dt);
                }
            }

            // Aerodynamic and rolling drag
            float drag = 0.0006f * vTang * std::abs(vTang) + 0.015f * vTang;
            m_chassisVel -= tangent * (drag * dt);
            if (!m_gasInput && !m_brakeInput) {
                w.angularVel = vTang / wheelR;
            }

            // Spawn dirt particles
            if ((m_gasInput || m_brakeInput) && std::abs(vTang) > 4.0f) {
                if ((std::rand() % 5) == 0) {
                    spawnDirt(mount - normal * wheelR, normal);
                }
            }
        } else {
            w.onGround = false;
            onGroundOut = false;
            w.suspensionOffset *= (1.0f - 12.0f * dt);
            if (m_gasInput && m_fuel > 0.0f) {
                w.angularVel += 40.0f * dt;
            } else {
                w.angularVel *= (1.0f - 2.0f * dt);
            }
        }

        // Rigid body connection: wheel position anchored to mount with subtle deflection
        w.pos = mount + localUp * w.suspensionOffset;
        w.vel = m_chassisVel;
        w.angle += w.angularVel * dt;
    };

    checkWheelGround(m_rearWheel, rearMount, rearOnGround);
    checkWheelGround(m_frontWheel, frontMount, frontOnGround);

    // Chassis Undercarriage Collision (belly plate protection against bottoming out)
    for (float localX : {-16.0f, 0.0f, 16.0f}) {
        Vec2 localPt(localX, -6.0f);
        Vec2 worldPt = m_chassisPos + rotate(localPt);
        float gh = getTerrainHeight(worldPt.x);
        float pen = gh - worldPt.y;
        if (pen > 0.0f) {
            Vec2 normal = getTerrainNormal(worldPt.x);
            m_chassisPos += normal * (pen * 0.4f);
            float vNorm = Vec2::dot(m_chassisVel, normal);
            if (vNorm < 0.0f) {
                m_chassisVel -= normal * (vNorm * 1.05f);
            }
            m_chassisVel.x *= (1.0f - 6.0f * dt);
        }
    }

    // Vehicle Attitude & Orientation Stability
    if (rearOnGround || frontOnGround) {
        // When wheels are on the ground: naturally align the car to the hill slope!
        float ghRear = getTerrainHeight(rearMount.x);
        float ghFront = getTerrainHeight(frontMount.x);
        float slopeAngle = std::atan2(ghFront - ghRear, frontMount.x - rearMount.x);
        float angleDiff = slopeAngle - m_chassisAngle;

        while (angleDiff > static_cast<float>(M_PI)) angleDiff -= 2.0f * static_cast<float>(M_PI);
        while (angleDiff < -static_cast<float>(M_PI)) angleDiff += 2.0f * static_cast<float>(M_PI);

        if (rearOnGround && frontOnGround) {
            // Both wheels down: firmly hugs the hill slope
            float alignTorque = angleDiff * 16.0f - m_chassisAngularVel * 6.5f;
            m_chassisAngularVel += alignTorque * dt;
        } else if (rearOnGround) {
            // Rear wheel on ground (wheelie): gravity pulls front wheel back down smoothly
            if (angleDiff < -0.1f) {
                m_chassisAngularVel += angleDiff * 8.0f * dt;
            }
        }

        // Throttle pitch reaction: gentle, satisfying wheelie lift that never flips the car on flat ground
        if (m_gasInput && m_fuel > 0.0f) {
            m_chassisAngularVel += 2.5f * dt; // gentle lift
        }
        if (m_brakeInput) {
            m_chassisAngularVel -= 3.5f * dt; // gentle nose down
        }
    } else {
        // Airborne: player has full air tilt control to perform flips or level out for landing
        if (m_gasInput && m_fuel > 0.0f) {
            m_chassisAngularVel += 4.5f * dt; // pitch back / backflip
            m_chassisVel += forwardDir * (50.0f * dt); // air forward momentum
        }
        if (m_brakeInput) {
            m_chassisAngularVel -= 4.5f * dt; // pitch forward / frontflip
        }
        m_chassisAngularVel *= (1.0f - 1.0f * dt); // smooth air rotation damping
    }

    // Integrate vehicle orientation and position
    m_chassisAngle += m_chassisAngularVel * dt;
    m_chassisPos += m_chassisVel * dt;

    // Driver head rigidly fixed to cockpit
    m_driverHeadPos = m_chassisPos + Vec2(-sinA * 22.0f, cosA * 22.0f);
    m_driverHeadVel = m_chassisVel;

    // Driver neck snap: ONLY triggers when car has completely rolled over onto its roof (tilted > 110 degrees)
    // and the roof/helmet hits the ground!
    float headGround = getTerrainHeight(m_driverHeadPos.x);
    bool isUpsideDown = (cosA < -0.35f);
    if (isUpsideDown && m_driverHeadPos.y <= headGround + 3.0f) {
        if (!m_isGameOver) {
            m_isGameOver = true;
            m_gameOverReason = "DRIVER DOWN! (NECK FLIP)";
            spawnSparkles(m_driverHeadPos, 0xFFFF3333);
        }
    }

    // Exhaust smoke when throttling
    if (m_gasInput && m_fuel > 0.0f && !m_isGameOver) {
        if ((std::rand() % 3) == 0) {
            Vec2 exhaustPipe = m_chassisPos + Vec2(-32.0f * cosA + 2.0f * sinA,
                                                   -32.0f * sinA - 2.0f * cosA);
            Vec2 exhaustDir(-cosA - 0.4f, -sinA + 0.3f);
            spawnSmoke(exhaustPipe, exhaustDir);
        }
    }
}

void GameEngine::updateCollectibles(float dt) {
    for (auto& item : m_collectibles) {
        if (item.collected) continue;

        item.animTimer += dt * 4.0f;

        // Collision check with vehicle chassis or wheels
        float dRear = (m_rearWheel.pos - item.pos).length();
        float dFront = (m_frontWheel.pos - item.pos).length();
        float dBody = (m_chassisPos - item.pos).length();

        if (dRear < 24.0f || dFront < 24.0f || dBody < 32.0f) {
            item.collected = true;
            if (item.type == Collectible::Coin) {
                m_score += item.value;
                m_coinsCollected++;
                spawnSparkles(item.pos, 0xFFFFD700);
                addFloatingText("+" + std::to_string(item.value), item.pos, 0xFFFFD700);
            } else if (item.type == Collectible::FuelCan) {
                m_fuel = std::min(100.0f, m_fuel + 65.0f);
                m_score += 100;
                spawnSparkles(item.pos, 0xFFFF3333);
                addFloatingText("+FUEL 100%", item.pos, 0xFF33FF33);
            }
        }
    }
}

void GameEngine::updateParticles(float dt) {
    // Update existing particles
    for (size_t i = 0; i < m_particles.size(); ) {
        m_particles[i].life -= dt;
        if (m_particles[i].life <= 0.0f) {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
        } else {
            m_particles[i].pos += m_particles[i].vel * dt;
            m_particles[i].vel.y -= 150.0f * dt; // particle gravity
            ++i;
        }
    }

    // Update floating texts
    for (size_t i = 0; i < m_floatingTexts.size(); ) {
        m_floatingTexts[i].life -= dt;
        if (m_floatingTexts[i].life <= 0.0f) {
            m_floatingTexts[i] = m_floatingTexts.back();
            m_floatingTexts.pop_back();
        } else {
            m_floatingTexts[i].pos.y += 20.0f * dt; // float upwards
            ++i;
        }
    }
}

void GameEngine::spawnSmoke(const Vec2& pos, const Vec2& dir) {
    Particle p;
    p.pos = pos;
    float speed = 25.0f + (std::rand() % 30);
    p.vel = dir.normalized() * speed + Vec2((std::rand() % 20 - 10), (std::rand() % 20 - 10));
    p.life = 0.5f + (std::rand() % 30) / 100.0f;
    p.maxLife = p.life;
    p.size = 3 + (std::rand() % 4);
    p.color = makeColor(180, 180, 190, 180);
    m_particles.push_back(p);
}

void GameEngine::spawnDirt(const Vec2& pos, const Vec2& normal) {
    for (int i = 0; i < 3; ++i) {
        Particle p;
        p.pos = pos;
        Vec2 sprayDir = normal + Vec2(-1.5f + (std::rand() % 10) / 10.0f, 0.5f + (std::rand() % 10) / 10.0f);
        p.vel = sprayDir.normalized() * (40.0f + (std::rand() % 60));
        p.life = 0.35f + (std::rand() % 20) / 100.0f;
        p.maxLife = p.life;
        p.size = 2;
        p.color = makeColor(110, 75, 45, 230);
        m_particles.push_back(p);
    }
}

void GameEngine::spawnSparkles(const Vec2& pos, uint32_t color) {
    for (int i = 0; i < 8; ++i) {
        Particle p;
        p.pos = pos;
        float angle = (i / 8.0f) * 2.0f * static_cast<float>(M_PI);
        float speed = 40.0f + (std::rand() % 50);
        p.vel = Vec2(std::cos(angle) * speed, std::sin(angle) * speed + 30.0f);
        p.life = 0.45f + (std::rand() % 20) / 100.0f;
        p.maxLife = p.life;
        p.size = 2;
        p.color = color;
        m_particles.push_back(p);
    }
}

void GameEngine::addFloatingText(const std::string& text, const Vec2& pos, uint32_t color) {
    FloatingText ft;
    ft.text = text;
    ft.pos = pos;
    ft.color = color;
    ft.life = 1.0f;
    ft.maxLife = 1.0f;
    m_floatingTexts.push_back(ft);
}

// Coordinate conversions
Vec2 GameEngine::worldToScreen(const Vec2& worldPt) const {
    float sx = (worldPt.x - m_cameraPos.x) + m_screenWidth * 0.35f;
    float sy = m_screenHeight * 0.65f - (worldPt.y - m_cameraPos.y);
    return Vec2(sx, sy);
}

Vec2 GameEngine::screenToWorld(const Vec2& screenPt) const {
    float wx = (screenPt.x - m_screenWidth * 0.35f) + m_cameraPos.x;
    float wy = m_cameraPos.y - (screenPt.y - m_screenHeight * 0.65f);
    return Vec2(wx, wy);
}

// Master Render Pipeline (100% Pure Rasterization)
void GameEngine::render(RasterBuffer& buffer) {
    renderSky(buffer);
    renderMountains(buffer);
    renderTerrain(buffer);
    renderCollectibles(buffer);
    renderParticles(buffer);
    renderVehicle(buffer);
    renderHUD(buffer);
    renderOverlays(buffer);
}

void GameEngine::renderSky(RasterBuffer& buffer) {
    // Scanline vertical sky gradient (deep vibrant blue to horizon cyan)
    int h = buffer.height();
    int w = buffer.width();

    for (int y = 0; y < h; ++y) {
        float t = static_cast<float>(y) / h;
        uint8_t r = static_cast<uint8_t>(50 + 130 * t);
        uint8_t g = static_cast<uint8_t>(130 + 90 * t);
        uint8_t b = static_cast<uint8_t>(230 + 25 * t);
        buffer.drawHorizontalSpan(y, 0, w - 1, makeColor(r, g, b));
    }

    // Rasterized Sun (Filled Midpoint Circle with rays)
    int sunX = w - 85;
    int sunY = 55;
    buffer.drawCircleFilled(sunX, sunY, 26, makeColor(255, 235, 80));
    buffer.drawCircleFilled(sunX, sunY, 21, makeColor(255, 250, 140));

    // Sun rays using Bresenham lines
    for (int i = 0; i < 8; ++i) {
        float angle = (i / 8.0f) * 2.0f * static_cast<float>(M_PI) + m_gameTime * 0.2f;
        int r1 = 30;
        int r2 = 38;
        int x1 = sunX + static_cast<int>(std::cos(angle) * r1);
        int y1 = sunY + static_cast<int>(std::sin(angle) * r1);
        int x2 = sunX + static_cast<int>(std::cos(angle) * r2);
        int y2 = sunY + static_cast<int>(std::sin(angle) * r2);
        buffer.drawLineBresenham(x1, y1, x2, y2, makeColor(255, 240, 100, 200), 2);
    }

    // Drifting raster clouds (overlapping filled circles)
    auto drawCloud = [&](int cx, int cy, int size) {
        uint32_t cloudColor = makeColor(255, 255, 255, 220);
        uint32_t cloudShadow = makeColor(220, 230, 245, 200);
        buffer.drawCircleFilled(cx, cy, size, cloudColor);
        buffer.drawCircleFilled(cx - size * 0.7f, cy + size * 0.2f, size * 0.75f, cloudColor);
        buffer.drawCircleFilled(cx + size * 0.7f, cy + size * 0.2f, size * 0.8f, cloudColor);
        buffer.drawCircleFilled(cx, cy + size * 0.4f, size * 0.6f, cloudShadow);
    };

    int c1X = static_cast<int>(std::fmod(w + 100 - m_cloudOffset * 0.7f, w + 200.0f)) - 100;
    int c2X = static_cast<int>(std::fmod(w + 350 - m_cloudOffset * 0.4f, w + 200.0f)) - 100;
    int c3X = static_cast<int>(std::fmod(w + 600 - m_cloudOffset * 0.9f, w + 200.0f)) - 100;
    drawCloud(c1X, 45, 22);
    drawCloud(c2X, 75, 28);
    drawCloud(c3X, 35, 18);
}

void GameEngine::renderMountains(RasterBuffer& buffer) {
    int w = buffer.width();
    int h = buffer.height();

    // Distant mountain silhouettes using parallax scanline polygon ridges
    float parallax1 = m_cameraPos.x * 0.08f;
    std::vector<QPoint> ridge1;
    ridge1.push_back(QPoint(0, h));

    for (int x = 0; x <= w; x += 16) {
        float worldX = (x + parallax1) * 2.0f;
        float my = 60.0f * std::sin(worldX * 0.002f) +
                   35.0f * std::sin(worldX * 0.006f + 1.2f) +
                   20.0f * std::cos(worldX * 0.015f);
        int screenY = static_cast<int>(h * 0.55f - my);
        ridge1.push_back(QPoint(x, screenY));
    }
    ridge1.push_back(QPoint(w, h));
    buffer.drawPolygonFilled(ridge1, makeColor(115, 150, 185));

    // Nearer mountain ridge
    float parallax2 = m_cameraPos.x * 0.18f;
    std::vector<QPoint> ridge2;
    ridge2.push_back(QPoint(0, h));

    for (int x = 0; x <= w; x += 12) {
        float worldX = (x + parallax2) * 1.5f;
        float my = 45.0f * std::sin(worldX * 0.0035f + 2.0f) +
                   25.0f * std::cos(worldX * 0.009f) +
                   15.0f * std::sin(worldX * 0.02f);
        int screenY = static_cast<int>(h * 0.62f - my);
        ridge2.push_back(QPoint(x, screenY));
    }
    ridge2.push_back(QPoint(w, h));
    buffer.drawPolygonFilled(ridge2, makeColor(90, 130, 160));
}

void GameEngine::renderTerrain(RasterBuffer& buffer) {
    int w = buffer.width();
    int h = buffer.height();

    // Render terrain column by column using fast vertical spans
    int prevY = -1;
    for (int px = 0; px < w; ++px) {
        Vec2 worldPt = screenToWorld(Vec2(static_cast<float>(px), 0.0f));
        float groundH = getTerrainHeight(worldPt.x);
        Vec2 screenGround = worldToScreen(Vec2(worldPt.x, groundH));
        int py = static_cast<int>(screenGround.y);

        if (py < h) {
            int startY = std::max(0, py);

            // Layer 1: Lush green grass crest (top 4-5 pixels)
            buffer.drawVerticalSpan(px, startY, std::min(h - 1, startY + 4), makeColor(46, 175, 40));

            // Layer 2: Warm topsoil / brown dirt (next 25-30 pixels)
            int dirtStart = startY + 5;
            int dirtEnd = std::min(h - 1, startY + 32);
            if (dirtStart <= dirtEnd) {
                // Occasional pebble raster variation
                uint32_t dirtCol = ((px ^ py) % 7 == 0) ? makeColor(150, 100, 60) : makeColor(128, 82, 45);
                buffer.drawVerticalSpan(px, dirtStart, dirtEnd, dirtCol);
            }

            // Layer 3: Deep bedrock / dark striated rock down to screen bottom
            int rockStart = dirtEnd + 1;
            if (rockStart < h) {
                for (int y = rockStart; y < h; ++y) {
                    uint32_t rockCol = ((y / 8) % 2 == 0) ? makeColor(65, 50, 42) : makeColor(52, 40, 34);
                    buffer.setPixel(px, y, rockCol);
                }
            }

            // Crisp dark grass boundary line along the ridge
            buffer.setPixel(px, py, makeColor(20, 80, 20));

            // Connect columns cleanly if steep slope with Bresenham line
            if (prevY != -1 && std::abs(py - prevY) > 1) {
                buffer.drawLineBresenham(px - 1, prevY, px, py, makeColor(20, 80, 20));
            }
        }
        prevY = py;
    }
}

void GameEngine::renderCollectibles(RasterBuffer& buffer) {
    for (const auto& item : m_collectibles) {
        if (item.collected) continue;

        Vec2 scr = worldToScreen(item.pos);
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        // Frustum culling
        if (sx < -30 || sx > buffer.width() + 30 || sy < -30 || sy > buffer.height() + 30) {
            continue;
        }

        if (item.type == Collectible::Coin) {
            // Spinning coin animation via oscillating width
            float spin = std::abs(std::cos(item.animTimer));
            int rX = std::max(2, static_cast<int>(10.0f * spin));
            int rY = 10;

            // Outer gold ring
            buffer.drawCircleFilled(sx, sy, rY, makeColor(220, 160, 0));
            // Inner gold sheen
            if (rX > 3) {
                buffer.drawCircleFilled(sx, sy, rY - 2, makeColor(255, 215, 0));
                // Center dollar mark / star
                buffer.drawLineBresenham(sx, sy - 4, sx, sy + 4, makeColor(180, 120, 0), 1);
            }
        } else if (item.type == Collectible::FuelCan) {
            // Raster red fuel canister
            int fw = 16, fh = 20;
            buffer.drawRectFilled(sx - fw / 2, sy - fh / 2, fw, fh, makeColor(210, 30, 30));
            buffer.drawRectOutline(sx - fw / 2, sy - fh / 2, fw, fh, makeColor(130, 10, 10));
            // Handle and spout
            buffer.drawRectFilled(sx - 4, sy - fh / 2 - 5, 8, 4, makeColor(140, 140, 140));
            buffer.drawRectFilled(sx + 3, sy - fh / 2 - 4, 3, 4, makeColor(240, 240, 40));
            // "F" label in white raster font
            buffer.drawBitmapChar('F', sx - 2, sy - 3, makeColor(255, 255, 255), 1);
        }
    }
}

void GameEngine::renderParticles(RasterBuffer& buffer) {
    for (const auto& p : m_particles) {
        Vec2 scr = worldToScreen(p.pos);
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        if (sx >= 0 && sx < buffer.width() && sy >= 0 && sy < buffer.height()) {
            float alphaProgress = p.life / p.maxLife;
            uint32_t col = p.color;
            uint8_t a = static_cast<uint8_t>(colorA(col) * alphaProgress);
            uint32_t fadedCol = makeColor(colorR(col), colorG(col), colorB(col), a);

            if (p.size <= 2) {
                buffer.setPixel(sx, sy, fadedCol);
                if (p.size == 2) {
                    buffer.setPixel(sx + 1, sy, fadedCol);
                    buffer.setPixel(sx, sy + 1, fadedCol);
                }
            } else {
                buffer.drawCircleFilled(sx, sy, p.size, fadedCol);
            }
        }
    }

    for (const auto& ft : m_floatingTexts) {
        Vec2 scr = worldToScreen(ft.pos);
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        if (sx > -100 && sx < buffer.width() + 100 && sy > -50 && sy < buffer.height() + 50) {
            float alpha = ft.life / ft.maxLife;
            uint32_t col = makeColor(colorR(ft.color), colorG(ft.color), colorB(ft.color),
                                     static_cast<uint8_t>(255 * alpha));
            buffer.drawBitmapString(ft.text, sx, sy, col, 1);
        }
    }
}

void GameEngine::renderVehicle(RasterBuffer& buffer) {
    float cosA = std::cos(m_chassisAngle);
    float sinA = std::sin(m_chassisAngle);

    // Chassis mount points
    Vec2 rearMount = m_chassisPos + Vec2(-m_wheelbaseHalf * cosA + 5.0f * sinA,
                                         -m_wheelbaseHalf * sinA - 5.0f * cosA);
    Vec2 frontMount = m_chassisPos + Vec2(m_wheelbaseHalf * cosA + 5.0f * sinA,
                                          m_wheelbaseHalf * sinA - 5.0f * cosA);

    // Render Suspension Springs using Bresenham lines
    auto drawSuspension = [&](const Vec2& mount, const Vec2& wheelPos) {
        Vec2 scrM = worldToScreen(mount);
        Vec2 scrW = worldToScreen(wheelPos);
        int x0 = static_cast<int>(scrM.x);
        int y0 = static_cast<int>(scrM.y);
        int x1 = static_cast<int>(scrW.x);
        int y1 = static_cast<int>(scrW.y);

        // Spring strut line + coils
        buffer.drawLineBresenham(x0, y0, x1, y1, makeColor(180, 180, 180), 2);

        // Damper cylinder
        int midX = (x0 + x1) / 2;
        int midY = (y0 + y1) / 2;
        buffer.drawCircleFilled(midX, midY, 3, makeColor(220, 40, 40));
    };

    drawSuspension(rearMount, m_rearWheel.pos);
    drawSuspension(frontMount, m_frontWheel.pos);

    // Render Chassis Body (Sporty Hill Climb Buggy)
    // Local coordinates of buggy body polygon
    const std::vector<Vec2> localBody = {
        Vec2(-32, -6),  // rear bumper
        Vec2(-32, 10),  // spoiler base
        Vec2(-38, 16),  // spoiler tip
        Vec2(-30, 14),  // spoiler top
        Vec2(-18, 10),  // cabin rear
        Vec2(-8, 22),   // roll cage top
        Vec2(10, 22),   // roof front
        Vec2(20, 8),    // windshield base
        Vec2(34, 4),    // hood front
        Vec2(34, -4),   // front bumper
        Vec2(24, -6),   // front fender
        Vec2(14, -2),   // front wheel cutout
        Vec2(-14, -2),  // side skirt
        Vec2(-24, -6)   // rear wheel cutout
    };

    std::vector<QPoint> screenBody;
    screenBody.reserve(localBody.size());
    for (const auto& pt : localBody) {
        Vec2 worldPt = m_chassisPos + Vec2(pt.x * cosA - pt.y * sinA,
                                           pt.x * sinA + pt.y * cosA);
        Vec2 scr = worldToScreen(worldPt);
        screenBody.push_back(QPoint(static_cast<int>(scr.x), static_cast<int>(scr.y)));
    }

    // Fill body shell with bright racing red
    buffer.drawPolygonFilled(screenBody, makeColor(230, 45, 45));

    // Dark body outline
    for (size_t i = 0; i < screenBody.size(); ++i) {
        size_t next = (i + 1) % screenBody.size();
        buffer.drawLineBresenham(screenBody[i].x(), screenBody[i].y(),
                                screenBody[next].x(), screenBody[next].y(),
                                makeColor(140, 15, 15), 2);
    }

    // Roll cage steel tubes using Bresenham lines
    Vec2 cageRearWorld = m_chassisPos + Vec2(-8.0f * cosA - 22.0f * sinA, -8.0f * sinA + 22.0f * cosA);
    Vec2 cageFrontWorld = m_chassisPos + Vec2(10.0f * cosA - 22.0f * sinA, 10.0f * sinA + 22.0f * cosA);
    Vec2 cageMountRear = m_chassisPos + Vec2(-18.0f * cosA - 10.0f * sinA, -18.0f * sinA + 10.0f * cosA);
    Vec2 windshieldMount = m_chassisPos + Vec2(20.0f * cosA - 8.0f * sinA, 20.0f * sinA + 8.0f * cosA);

    Vec2 scrCR = worldToScreen(cageRearWorld);
    Vec2 scrCF = worldToScreen(cageFrontWorld);
    Vec2 scrMR = worldToScreen(cageMountRear);
    Vec2 scrWM = worldToScreen(windshieldMount);

    uint32_t steelCol = makeColor(60, 60, 70);
    buffer.drawLineBresenham(scrMR.x, scrMR.y, scrCR.x, scrCR.y, steelCol, 2);
    buffer.drawLineBresenham(scrCR.x, scrCR.y, scrCF.x, scrCF.y, steelCol, 2);
    buffer.drawLineBresenham(scrCF.x, scrCF.y, scrWM.x, scrWM.y, steelCol, 2);
    // Cross brace
    buffer.drawLineBresenham(scrMR.x, scrMR.y, scrCF.x, scrCF.y, steelCol, 1);

    // Windshield glass tint
    buffer.drawLineBresenham(scrCF.x - 1, scrCF.y + 2, scrWM.x - 1, scrWM.y - 2, makeColor(140, 220, 240, 180), 2);

    // Render Driver
    Vec2 driverBodyWorld = m_chassisPos + Vec2(0.0f * cosA - 8.0f * sinA, 0.0f * sinA + 8.0f * cosA);
    Vec2 scrDB = worldToScreen(driverBodyWorld);
    Vec2 scrHead = worldToScreen(m_driverHeadPos);

    // Racing suit body
    buffer.drawCircleFilled(scrDB.x, scrDB.y, 6, makeColor(30, 90, 220));

    // Helmet (bright yellow with black visor)
    int headR = 7;
    buffer.drawCircleFilled(scrHead.x, scrHead.y, headR, makeColor(255, 215, 0));
    buffer.drawCircleMidpoint(scrHead.x, scrHead.y, headR, makeColor(180, 140, 0));
    // Visor looking forward
    int visorX = scrHead.x + static_cast<int>(3.0f * cosA);
    int visorY = scrHead.y - static_cast<int>(3.0f * sinA);
    buffer.drawCircleFilled(visorX, visorY, 3, makeColor(20, 20, 25));

    // Hands on steering wheel
    Vec2 steerWorld = m_chassisPos + Vec2(12.0f * cosA - 8.0f * sinA, 12.0f * sinA + 8.0f * cosA);
    Vec2 scrSteer = worldToScreen(steerWorld);
    buffer.drawLineBresenham(scrDB.x + 3, scrDB.y - 1, scrSteer.x, scrSteer.y, makeColor(30, 90, 220), 2);
    buffer.drawCircleFilled(scrSteer.x, scrSteer.y, 3, makeColor(40, 40, 40));

    // Headlight cone at night/dusk or front
    Vec2 headLightPos = m_chassisPos + Vec2(35.0f * cosA - 2.0f * sinA, 35.0f * sinA + 2.0f * cosA);
    Vec2 scrHL = worldToScreen(headLightPos);
    buffer.drawCircleFilled(scrHL.x, scrHL.y, 3, makeColor(255, 255, 180));

    // Render Wheels (Rear and Front)
    auto drawWheel = [&](const Wheel& w) {
        Vec2 scrW = worldToScreen(w.pos);
        int wx = static_cast<int>(scrW.x);
        int wy = static_cast<int>(scrW.y);
        int r = static_cast<int>(w.radius);

        // 1. Thick rubber outer tire
        buffer.drawCircleFilled(wx, wy, r, makeColor(35, 35, 40));
        buffer.drawCircleThick(wx, wy, r - 1, 3, makeColor(20, 20, 25));

        // 2. Tire treads along rim
        for (int i = 0; i < 8; ++i) {
            float a = w.angle + (i / 8.0f) * 2.0f * static_cast<float>(M_PI);
            int tx = wx + static_cast<int>(std::cos(a) * (r - 2));
            int ty = wy - static_cast<int>(std::sin(a) * (r - 2));
            buffer.setPixel(tx, ty, makeColor(60, 60, 65));
        }

        // 3. Metallic wheel rim
        int rimR = r - 5;
        buffer.drawCircleFilled(wx, wy, rimR, makeColor(170, 175, 180));
        buffer.drawCircleMidpoint(wx, wy, rimR, makeColor(110, 115, 120));

        // 4. Rotating wheel spokes (Bresenham lines showing dynamic wheel spinning!)
        for (int i = 0; i < 4; ++i) {
            float a = w.angle + (i / 4.0f) * static_cast<float>(M_PI);
            int xA = wx + static_cast<int>(std::cos(a) * (rimR - 1));
            int yA = wy - static_cast<int>(std::sin(a) * (rimR - 1));
            int xB = wx - static_cast<int>(std::cos(a) * (rimR - 1));
            int yB = wy + static_cast<int>(std::sin(a) * (rimR - 1));
            buffer.drawLineBresenham(xA, yA, xB, yB, makeColor(70, 75, 80), 2);
        }

        // 5. Center wheel hub cap
        buffer.drawCircleFilled(wx, wy, 3, makeColor(220, 50, 50));
    };

    drawWheel(m_rearWheel);
    drawWheel(m_frontWheel);
}

void GameEngine::renderHUD(RasterBuffer& buffer) {
    int w = buffer.width();

    // Top Dark HUD Banner
    buffer.drawRectFilled(0, 0, w, 32, makeColor(15, 20, 30, 210));
    buffer.drawLineBresenham(0, 32, w - 1, 32, makeColor(50, 65, 90), 1);

    // 1. Distance meter
    std::string distStr = "DIST: " + std::to_string(static_cast<int>(m_maxDistance)) + "m";
    buffer.drawBitmapString(distStr, 12, 10, makeColor(255, 255, 255), 2);

    // 2. Coins collected
    std::string coinStr = "COINS: " + std::to_string(m_coinsCollected);
    buffer.drawBitmapString(coinStr, 180, 10, makeColor(255, 215, 0), 2);

    // 3. Score
    std::string scoreStr = "SCORE: " + std::to_string(m_score);
    buffer.drawBitmapString(scoreStr, 320, 10, makeColor(100, 230, 255), 2);

    // 4. Fuel Gauge in Top Right
    int fuelBarX = w - 160;
    int fuelBarY = 8;
    int fuelBarW = 100;
    int fuelBarH = 14;

    buffer.drawBitmapString("FUEL", fuelBarX - 42, 10, makeColor(240, 240, 240), 2);
    buffer.drawRectFilled(fuelBarX, fuelBarY, fuelBarW, fuelBarH, makeColor(40, 45, 50));
    buffer.drawRectOutline(fuelBarX, fuelBarY, fuelBarW, fuelBarH, makeColor(180, 180, 190));

    int currentFuelW = static_cast<int>((m_fuel / 100.0f) * (fuelBarW - 2));
    uint32_t fuelCol = makeColor(50, 220, 50);
    if (m_fuel < 20.0f) {
        // Flashing red warning when low
        bool flash = (static_cast<int>(m_gameTime * 6.0f) % 2 == 0);
        fuelCol = flash ? makeColor(255, 30, 30) : makeColor(140, 20, 20);
        buffer.drawBitmapString("LOW!", fuelBarX + fuelBarW + 6, 10, makeColor(255, 50, 50), 2);
    } else if (m_fuel < 50.0f) {
        fuelCol = makeColor(240, 200, 30);
    }
    if (currentFuelW > 0) {
        buffer.drawRectFilled(fuelBarX + 1, fuelBarY + 1, currentFuelW, fuelBarH - 2, fuelCol);
    }

    // Speedometer / Gauge in bottom right
    int gaugeX = w - 50;
    int gaugeY = buffer.height() - 45;
    int gaugeR = 24;

    buffer.drawCircleFilled(gaugeX, gaugeY, gaugeR, makeColor(20, 25, 35, 200));
    buffer.drawCircleMidpoint(gaugeX, gaugeY, gaugeR, makeColor(100, 120, 150));
    // Speedometer needle
    float speedRatio = std::min(1.0f, getSpeedKmh() / 140.0f);
    float needleAngle = static_cast<float>(M_PI * 0.75 - speedRatio * M_PI * 1.5);
    int nx = gaugeX + static_cast<int>(std::cos(needleAngle) * (gaugeR - 4));
    int ny = gaugeY - static_cast<int>(std::sin(needleAngle) * (gaugeR - 4));
    buffer.drawLineBresenham(gaugeX, gaugeY, nx, ny, makeColor(255, 50, 50), 2);
    buffer.drawCircleFilled(gaugeX, gaugeY, 3, makeColor(255, 255, 255));

    std::string speedStr = std::to_string(static_cast<int>(getSpeedKmh())) + " KM/H";
    buffer.drawBitmapString(speedStr, gaugeX - 25, gaugeY + gaugeR + 4, makeColor(220, 230, 245), 1);

    // Bottom Controls Reminder
    int ctrlY = buffer.height() - 20;
    buffer.drawBitmapString("[D / ->] GAS    [A / <-] BRAKE    [R] RESTART    [G] GRID    [F11] FULLSCREEN",
                            15, ctrlY, makeColor(20, 25, 35), 1);
}

void GameEngine::renderOverlays(RasterBuffer& buffer) {
    int w = buffer.width();
    int h = buffer.height();

    if (m_isGameOver) {
        // Darkened raster backdrop
        buffer.drawRectFilled(0, 0, w, h, makeColor(0, 0, 0, 175));

        // Game Over modal box
        int boxW = 340;
        int boxH = 170;
        int boxX = (w - boxW) / 2;
        int boxY = (h - boxH) / 2;

        buffer.drawRectFilled(boxX, boxY, boxW, boxH, makeColor(25, 30, 42, 245));
        buffer.drawRectOutline(boxX, boxY, boxW, boxH, makeColor(220, 60, 60));
        buffer.drawRectOutline(boxX + 2, boxY + 2, boxW - 4, boxH - 4, makeColor(80, 90, 110));

        // Header
        buffer.drawBitmapString("GAME OVER", boxX + 85, boxY + 18, makeColor(255, 60, 60), 3);
        buffer.drawBitmapString(m_gameOverReason, boxX + 35, boxY + 52, makeColor(255, 200, 80), 2);

        // Stats
        std::string distStr = "Distance Reached: " + std::to_string(static_cast<int>(m_maxDistance)) + "m";
        std::string coinsStr = "Coins Collected:  " + std::to_string(m_coinsCollected);
        std::string scoreStr = "Total Score:      " + std::to_string(m_score);

        buffer.drawBitmapString(distStr, boxX + 40, boxY + 80, makeColor(240, 240, 240), 1);
        buffer.drawBitmapString(coinsStr, boxX + 40, boxY + 95, makeColor(255, 215, 0), 1);
        buffer.drawBitmapString(scoreStr, boxX + 40, boxY + 110, makeColor(100, 230, 255), 1);

        // Prompt
        buffer.drawBitmapString("PRESS [R] OR CLICK RESTART TO RETRY", boxX + 25, boxY + 138, makeColor(140, 255, 140), 1);
    } else if (m_isPaused) {
        buffer.drawRectFilled(0, 0, w, h, makeColor(0, 0, 0, 150));
        int boxW = 280;
        int boxH = 90;
        int boxX = (w - boxW) / 2;
        int boxY = (h - boxH) / 2;
        buffer.drawRectFilled(boxX, boxY, boxW, boxH, makeColor(30, 35, 50, 240));
        buffer.drawRectOutline(boxX, boxY, boxW, boxH, makeColor(100, 150, 220));
        buffer.drawBitmapString("PAUSED", boxX + 85, boxY + 20, makeColor(255, 255, 255), 3);
        buffer.drawBitmapString("PRESS [SPACE] OR [P] TO RESUME", boxX + 30, boxY + 60, makeColor(200, 220, 255), 1);
    }
}
