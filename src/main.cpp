#include <Geode/Geode.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/loader/Log.hpp>
#include <ctime>
#include <string_view>

using namespace geode::prelude;
using namespace std::literals;

// ================== SISTEMA DE RACHAS ==================
struct StreakData {
    int currentStreak = 0;
    int starsToday = 0;
    bool hasNewStreak = false;
    std::string lastDay = "";

    // Función para obtener las estrellas requeridas según la racha
    int getRequiredStars() {
        if (currentStreak >= 81) return 21;      // racha9.png
        else if (currentStreak >= 71) return 19; // racha8.png
        else if (currentStreak >= 61) return 17; // racha7.png
        else if (currentStreak >= 51) return 15; // racha6.png
        else if (currentStreak >= 41) return 13; // racha5.png
        else if (currentStreak >= 31) return 11; // racha4.png
        else if (currentStreak >= 21) return 9;  // racha3.png
        else if (currentStreak >= 11) return 7;  // racha2.png
        else if (currentStreak >= 1) return 5;   // racha1.png
        else return 5;                           // racha0.png
    }

    // Función para obtener las estrellas requeridas por nivel de racha
    int getRequiredStarsForLevel(int streakLevel) {
        if (streakLevel >= 81) return 21;      // racha9.png
        else if (streakLevel >= 71) return 19; // racha8.png
        else if (streakLevel >= 61) return 17; // racha7.png
        else if (streakLevel >= 51) return 15; // racha6.png
        else if (streakLevel >= 41) return 13; // racha5.png
        else if (streakLevel >= 31) return 11; // racha4.png
        else if (streakLevel >= 21) return 9;  // racha3.png
        else if (streakLevel >= 11) return 7;  // racha2.png
        else if (streakLevel >= 1) return 5;   // racha1.png
        else return 5;                         // racha0.png
    }

    void load() {
        currentStreak = Mod::get()->getSavedValue<int>("streak", 0);
        starsToday = Mod::get()->getSavedValue<int>("starsToday", 0);
        hasNewStreak = Mod::get()->getSavedValue<bool>("hasNewStreak", false);
        lastDay = Mod::get()->getSavedValue<std::string>("lastDay", "");
        dailyUpdate();
    }

    void save() {
        Mod::get()->setSavedValue<int>("streak", currentStreak);
        Mod::get()->setSavedValue<int>("starsToday", starsToday);
        Mod::get()->setSavedValue<bool>("hasNewStreak", hasNewStreak);
        Mod::get()->setSavedValue<std::string>("lastDay", lastDay);
    }

    std::string getCurrentDate() {
        time_t t = time(nullptr);
        tm* now = localtime(&t);
        char buf[16];
        strftime(buf, sizeof(buf), "%Y-%m-%d", now);
        return std::string(buf);
    }

    void dailyUpdate() {
        std::string today = getCurrentDate();
        if (lastDay != today) {
            int requiredStars = getRequiredStars();
            if (starsToday < requiredStars && !lastDay.empty()) {
                currentStreak = 0;
            }
            starsToday = 0;
            lastDay = today;
            save();
        }
    }

    void addStars(int count) {
        load();
        dailyUpdate();
        
        int oldStreak = currentStreak;
        int requiredStars = getRequiredStars();
        bool alreadyGotRacha = (starsToday >= requiredStars);
        starsToday += count;
        
        if (!alreadyGotRacha && starsToday >= requiredStars) {
            currentStreak++;
            // Marcar que hay una nueva racha para mostrar en el popup
            hasNewStreak = true;
        }
        
        save();
    }

    bool shouldShowAnimation() {
        if (hasNewStreak) {
            hasNewStreak = false;
            save();
            return true;
        }
        return false;
    }

    std::string getRachaSprite() {
        int streak = currentStreak;
        if (streak >= 81) return "racha9.png"_spr;
        else if (streak >= 71) return "racha8.png"_spr;
        else if (streak >= 61) return "racha7.png"_spr;
        else if (streak >= 51) return "racha6.png"_spr;
        else if (streak >= 41) return "racha5.png"_spr;
        else if (streak >= 31) return "racha4.png"_spr;
        else if (streak >= 21) return "racha3.png"_spr;
        else if (streak >= 11) return "racha2.png"_spr;
        else if (streak >= 1)  return "racha1.png"_spr;
        else return "racha0.png"_spr;
    }

    std::string getRachaSpriteForAnimation() {
        int streak = currentStreak;
        if (streak >= 81) return "racha9.png"_spr;
        else if (streak >= 71) return "racha8.png"_spr;
        else if (streak >= 61) return "racha7.png"_spr;
        else if (streak >= 51) return "racha6.png"_spr;
        else if (streak >= 41) return "racha5.png"_spr;
        else if (streak >= 31) return "racha4.png"_spr;
        else if (streak >= 21) return "racha3.png"_spr;
        else if (streak >= 11) return "racha2.png"_spr;
        else if (streak >= 1)  return "racha1.png"_spr;
        else return "racha0.png"_spr;
    }
};

StreakData g_streakData;

// ============= HOOK PARA CONTAR ESTRELLAS =============
class $modify(MyGameStatsManager, GameStatsManager) {
    void incrementStat(char const* p0, int p1) {
        if (std::string_view(p0) == "6"sv) {
            g_streakData.addStars(p1);
        }
        GameStatsManager::incrementStat(p0, p1);
    }
};

// ============= POPUP QUE MUESTRA TODAS LAS RACHAS =============
class AllRachasPopup : public Popup<> {
protected:
    bool setup() override {
        this->setTitle("All Streaks");
        auto winSize = m_mainLayer->getContentSize();

        float startX = 50.f;
        float y = winSize.height / 2 + 20.f; // Ajustar posición Y un poco más arriba
        float spacing = 50.f;

        // Lista de rachas con sus días y estrellas requeridas
        std::vector<std::tuple<std::string, int, int>> rachas = {
            { "racha1.png"_spr, 1, 5 },
            { "racha2.png"_spr, 11, 7 },
            { "racha3.png"_spr, 21, 9 },
            { "racha4.png"_spr, 31, 11 },
            { "racha5.png"_spr, 41, 13 },
            { "racha6.png"_spr, 51, 15 },
            { "racha7.png"_spr, 61, 17 },
            { "racha8.png"_spr, 71, 19 },
            { "racha9.png"_spr, 81, 21 }
        };

        int i = 0;
        for (auto& [sprite, day, requiredStars] : rachas) {
            auto spr = CCSprite::create(sprite.c_str());
            spr->setScale(0.22f); // Escala un poco más pequeña
            spr->setPosition({ startX + i * spacing, y });
            m_mainLayer->addChild(spr);

            // Texto del día
            auto label = CCLabelBMFont::create(
                CCString::createWithFormat("Day %d", day)->getCString(),
                "goldFont.fnt"
            );
            label->setScale(0.35f); // Escala un poco más pequeña
            label->setPosition({ startX + i * spacing, y - 40 });
            m_mainLayer->addChild(label);

            // Icono de estrella y texto de estrellas requeridas (más juntos)
            auto starIcon = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
            starIcon->setScale(0.4f); // Escala un poco más pequeña
            starIcon->setPosition({ startX + i * spacing - 4, y - 60 });
            m_mainLayer->addChild(starIcon);

            auto starsLabel = CCLabelBMFont::create(
                CCString::createWithFormat("%d", requiredStars)->getCString(),
                "bigFont.fnt"
            );
            starsLabel->setScale(0.3f); // Escala un poco más pequeña
            starsLabel->setPosition({ startX + i * spacing + 6, y - 60 });
            m_mainLayer->addChild(starsLabel);

            i++;
        }

        return true;
    }

public:
    static AllRachasPopup* create() {
        auto ret = new AllRachasPopup();
        if (ret && ret->initAnchored(500.f, 155.f)) { // Altura reducida de 200 a 180
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
// =========== POPUP PRINCIPAL ==============
class InfoPopup : public Popup<> {
protected:
    bool setup() override {
        g_streakData.load();
        g_streakData.dailyUpdate();

        this->setTitle("Current Streak");
        auto winSize = m_mainLayer->getContentSize();
        float centerY = winSize.height / 2 + 25;

        // Sprite de racha
        auto spriteName = g_streakData.getRachaSprite();
        auto rachaSprite = CCSprite::create(spriteName.c_str());
        rachaSprite->setScale(0.4f);

        auto rachaBtn = CCMenuItemSpriteExtra::create(
            rachaSprite, this, menu_selector(InfoPopup::onRachaClick)
        );

        auto menuRacha = CCMenu::create();
        menuRacha->addChild(rachaBtn);
        menuRacha->setPosition({ winSize.width / 2, centerY });
        m_mainLayer->addChild(menuRacha, 3);

        // Animación de levitación
        auto floatUp = CCMoveBy::create(1.5f, { 0, 8 });
        auto floatDown = floatUp->reverse();
        auto seq = CCSequence::create(floatUp, floatDown, nullptr);
        auto repeat = CCRepeatForever::create(seq);
        rachaSprite->runAction(repeat);

        // Texto de racha
        auto streakLabel = CCLabelBMFont::create(
            ("Daily streak: " + std::to_string(g_streakData.currentStreak)).c_str(),
            "goldFont.fnt"
        );
        streakLabel->setScale(0.55f);
        streakLabel->setPosition({ winSize.width / 2, centerY - 60 });
        m_mainLayer->addChild(streakLabel);

        // Barra de progreso
        float barWidth = 140.0f;
        float barHeight = 16.0f;
        int requiredStars = g_streakData.getRequiredStars();
        float percent = std::min(g_streakData.starsToday / static_cast<float>(requiredStars), 1.0f);

        auto barBg = CCLayerColor::create({ 45, 45, 45, 255 }, barWidth, barHeight);
        barBg->setPosition({ winSize.width / 2 - barWidth / 2, centerY - 90 });
        m_mainLayer->addChild(barBg, 1);

        auto barFg = CCLayerGradient::create({ 250, 225, 60, 255 }, { 255, 165, 0, 255 });
        barFg->setContentSize({ barWidth * percent, barHeight });
        barFg->setPosition({ winSize.width / 2 - barWidth / 2, centerY - 90 });
        m_mainLayer->addChild(barFg, 2);

        auto border = CCLayerColor::create({ 255,255,255,255 }, barWidth + 2, barHeight + 2);
        border->setPosition({ winSize.width / 2 - barWidth / 2 - 1, centerY - 91 });
        border->setZOrder(4);
        border->setOpacity(120);
        m_mainLayer->addChild(border);

        auto outer = CCLayerColor::create({ 0,0,0,255 }, barWidth + 6, barHeight + 6);
        outer->setPosition({ winSize.width / 2 - barWidth / 2 - 3, centerY - 93 });
        outer->setZOrder(0);
        outer->setOpacity(70);
        m_mainLayer->addChild(outer);

        // Icono de estrella
        auto starIcon = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        starIcon->setScale(0.45f);
        starIcon->setPosition({ winSize.width / 2 - 25, centerY - 82 });
        m_mainLayer->addChild(starIcon, 5);

        // Texto del contador de estrellas
        auto barText = CCLabelBMFont::create(
            (std::to_string(g_streakData.starsToday) + " / " + std::to_string(requiredStars)).c_str(),
            "bigFont.fnt"
        );
        barText->setScale(0.45f);
        barText->setPosition({ winSize.width / 2 + 15, centerY - 82 });
        m_mainLayer->addChild(barText, 5);

        // INDICADOR DE RACHA AL LADO DERECHO DE LA BARRA
        std::string indicatorSpriteName;
        if (g_streakData.starsToday >= requiredStars) {
            // Si ya completó la racha hoy, mostrar la racha actual
            indicatorSpriteName = g_streakData.getRachaSprite();
        }
        else {
            // Si no ha completado la racha hoy, mostrar racha0.png
            indicatorSpriteName = "racha0.png"_spr;
        }

        auto rachaIndicator = CCSprite::create(indicatorSpriteName.c_str());
        rachaIndicator->setScale(0.14f); // Sprite más pequeño
        rachaIndicator->setPosition({ winSize.width / 2 + barWidth / 2 + 20, centerY - 82 });
        m_mainLayer->addChild(rachaIndicator, 5);

        // Botón de información
        auto infoIcon = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        infoIcon->setScale(0.6f);

        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoIcon, this, menu_selector(InfoPopup::onInfo)
        );

        auto menu = CCMenu::create();
        menu->setPosition({ winSize.width - 20, winSize.height - 20 });
        menu->addChild(infoBtn);
        m_mainLayer->addChild(menu, 10);

        // Mostrar animación si hay nueva racha
        if (g_streakData.shouldShowAnimation()) {
            this->showStreakAnimation(g_streakData.currentStreak);
        }

        return true;
    }

    void onInfo(CCObject*) {
        FLAlertLayer::create(
            "About Racha!",
            "Collect 5 stars every day to increase your streak!\n"
            "If you miss a day (less than 5 stars), your streak resets.\n\n"
            "Icons change depending on how many days you keep your streak.",
            "OK"
        )->show();
    
    }

    void onRachaClick(CCObject*) {
        AllRachasPopup::create()->show();
    }

    void showStreakAnimation(int streakLevel) {
        auto winSize = this->getContentSize();
        
        // Crear capa para la animación dentro del popup
        auto animLayer = CCLayer::create();
        animLayer->setContentSize(winSize);
        animLayer->setPosition(0, 0);
        animLayer->setZOrder(1000); // Alto z-order dentro del popup
        this->addChild(animLayer);
        
        // Fondo semitransparente
        auto bg = CCLayerColor::create(ccc4(0, 0, 0, 180), winSize.width, winSize.height);
        bg->setPosition(0, 0);
        animLayer->addChild(bg);
        
        // Determinar el sprite de la racha que se desbloqueó
        std::string spriteName = g_streakData.getRachaSpriteForAnimation();
        
        // Sprite principal de la racha
        auto rachaSprite = CCSprite::create(spriteName.c_str());
        rachaSprite->setPosition(CCPoint(winSize.width / 2, winSize.height / 2));
        rachaSprite->setScale(0.1f);
        animLayer->addChild(rachaSprite);
        
        // Sprite de "New Streak" - Mantener la animación original
        auto newStreakSprite = CCSprite::create("NewStreak.png"_spr);
        if (!newStreakSprite) {
            // Si no existe el sprite, crear texto como fallback
            newStreakSprite = CCSprite::create();
            auto fallbackText = CCLabelBMFont::create("New Streak!", "bigFont.fnt");
            fallbackText->setColor(ccc3(255, 255, 0));
            newStreakSprite->addChild(fallbackText);
        }
        newStreakSprite->setPosition(CCPoint(winSize.width / 2, winSize.height / 2 + 100));
        newStreakSprite->setScale(0.1f);
        animLayer->addChild(newStreakSprite);
        
        // Número de días
        auto daysLabel = CCLabelBMFont::create(
            CCString::createWithFormat("Day %d", streakLevel)->getCString(), 
            "goldFont.fnt"
        );
        daysLabel->setPosition(CCPoint(winSize.width / 2, winSize.height / 2 - 80));
        daysLabel->setScale(0.8f);
        daysLabel->setColor(ccc3(255, 215, 0));
        animLayer->addChild(daysLabel);
        
        // Animación de aparición
        auto scaleUp = CCScaleTo::create(0.8f, 1.2f);
        auto scaleDown = CCScaleTo::create(0.3f, 1.0f);
        auto scaleSequence = CCSequence::create(scaleUp, scaleDown, nullptr);
        
        auto rotate = CCRotateBy::create(1.5f, 360);
        auto easeRotate = CCEaseElasticOut::create(rotate, 0.8f);
        
        auto spawn = CCSpawn::create(scaleSequence, easeRotate, nullptr);
        
        // Ejecutar animaciones
        rachaSprite->runAction(spawn);
        
        // Animación para el sprite "New Streak"
        newStreakSprite->runAction(
            CCSequence::create(
                CCScaleTo::create(0.5f, 1.2f),
                CCScaleTo::create(0.2f, 1.0f),
                CCRepeatForever::create(
                    CCSequence::create(
                        CCScaleTo::create(0.5f, 1.1f),
                        CCScaleTo::create(0.5f, 1.0f),
                        nullptr
                    )
                ),
                nullptr
            )
        );
        
        // Animación para el número de días (parpadeo)
        daysLabel->runAction(
            CCRepeatForever::create(
                CCSequence::create(
                    CCFadeTo::create(0.5f, 150),
                    CCFadeTo::create(0.5f, 255),
                    nullptr
                )
            )
        );
        
        // Efecto de brillo
        for (int i = 0; i < 3; i++) {
            auto glow = CCSprite::create();
            glow->setTextureRect(CCRect(0, 0, 200, 200));
            glow->setColor(ccc3(255, 255, 100));
            glow->setPosition(CCPoint(winSize.width / 2, winSize.height / 2));
            glow->setScale(0.5f + i * 0.3f);
            glow->setOpacity(100 - i * 30);
            glow->setBlendFunc(ccBlendFunc{GL_SRC_ALPHA, GL_ONE});
            animLayer->addChild(glow, -1);
            
            auto glowScaleUp = CCScaleTo::create(1.0f, 0.8f + i * 0.4f);
            auto glowScaleDown = CCScaleTo::create(1.0f, 0.6f + i * 0.3f);
            auto glowFadeUp = CCFadeTo::create(1.0f, 120 - i * 30);
            auto glowFadeDown = CCFadeTo::create(1.0f, 80 - i * 20);
            
            glow->runAction(
                CCRepeatForever::create(
                    CCSequence::create(
                        CCSpawn::create(glowScaleUp, glowFadeUp, nullptr),
                        CCSpawn::create(glowScaleDown, glowFadeDown, nullptr),
                        nullptr
                    )
                )
            );
        }
        
        // Fade lento para desaparecer - Crear acciones separadas para cada elemento
        CCArray* children = animLayer->getChildren();
        for (int i = 0; i < children->count(); i++) {
            auto child = dynamic_cast<CCNode*>(children->objectAtIndex(i));
            if (child) {
                // Crear una nueva acción de fade para cada elemento
                auto individualFade = CCFadeOut::create(1.5f);
                child->runAction(
                    CCSequence::create(
                        CCDelayTime::create(3.0f),
                        individualFade,
                        nullptr
                    )
                );
            }
        }
        
        // Quitar la animación después de un tiempo con fade lento
        animLayer->runAction(
            CCSequence::create(
                CCDelayTime::create(4.5f), // 3s de animación + 1.5s de fade
                CCCallFunc::create(animLayer, callfunc_selector(CCNode::removeFromParent)),
                nullptr
            )
        );
    }

public:
    static InfoPopup* create() {
        auto ret = new InfoPopup();
        if (ret && ret->initAnchored(260.f, 220.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ============ BOTÓN EN MENÚ PRINCIPAL ==================
class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        g_streakData.load();
        g_streakData.dailyUpdate();

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return false;

        auto spriteName = g_streakData.getRachaSprite();
        auto icon = CCSprite::create(spriteName.c_str());
        icon->setScale(0.5f);

        auto circle = CircleButtonSprite::create(
            icon, CircleBaseColor::Green, CircleBaseSize::Medium
        );

        auto btn = CCMenuItemSpriteExtra::create(
            circle, this, menu_selector(MyMenuLayer::onOpenPopup)
        );

        btn->setPositionY(btn->getPositionY() + 5);
        menu->addChild(btn);
        menu->updateLayout();

        return true;
    }

    void onOpenPopup(CCObject*) {
        InfoPopup::create()->show();
    }
};