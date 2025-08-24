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
    std::string lastDay = "";

    void load() {
        currentStreak = Mod::get()->getSavedValue<int>("streak", 0);
        starsToday = Mod::get()->getSavedValue<int>("starsToday", 0);
        lastDay = Mod::get()->getSavedValue<std::string>("lastDay", "");
        dailyUpdate(); // Verificar cambio de día al cargar
    }

    void save() {
        Mod::get()->setSavedValue<int>("streak", currentStreak);
        Mod::get()->setSavedValue<int>("starsToday", starsToday);
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
            if (starsToday < 5 && !lastDay.empty()) {
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
        bool alreadyGotRacha = (starsToday >= 5);
        starsToday += count;
        if (!alreadyGotRacha && starsToday >= 5) {
            currentStreak++;
        }
        save();
    }

    // ----------- SELECCIÓN DE SPRITE SEGÚN RACHA -----------
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
        else return "";
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
        float y = winSize.height / 2;
        float spacing = 50.f;

        // Lista de sprites + días
        std::vector<std::pair<std::string, int>> rachas = {
            { "racha1.png"_spr, 1 },
            { "racha2.png"_spr, 11 },
            { "racha3.png"_spr, 21 },
            { "racha4.png"_spr, 31 },
            { "racha5.png"_spr, 41 },
            { "racha6.png"_spr, 51 },
            { "racha7.png"_spr, 61 },
            { "racha8.png"_spr, 71 },
            { "racha9.png"_spr, 81 }
        };

        int i = 0;
        for (auto& [sprite, day] : rachas) {
            auto spr = CCSprite::create(sprite.c_str());
            spr->setScale(0.25f);
            spr->setPosition({ startX + i * spacing, y });
            m_mainLayer->addChild(spr);

            auto label = CCLabelBMFont::create(
                CCString::createWithFormat("Day %d", day)->getCString(),
                "goldFont.fnt"
            );
            label->setScale(0.4f);
            label->setPosition({ startX + i * spacing, y - 45 });
            m_mainLayer->addChild(label);

            i++;
        }

        return true;
    }

public:
    static AllRachasPopup* create() {
        auto ret = new AllRachasPopup();
        if (ret && ret->initAnchored(500.f, 150.f)) { // ancho > alto
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

        // -------- SPRITE DE RACHA CLICKEABLE --------
        auto spriteName = g_streakData.getRachaSprite();
        if (!spriteName.empty()) {
            auto rachaSprite = CCSprite::create(spriteName.c_str());
            rachaSprite->setScale(0.4f);

            auto rachaBtn = CCMenuItemSpriteExtra::create(
                rachaSprite,
                this,
                menu_selector(InfoPopup::onRachaClick)
            );

            auto menuRacha = CCMenu::create();
            menuRacha->addChild(rachaBtn);
            menuRacha->setPosition({ winSize.width / 2, centerY });
            m_mainLayer->addChild(menuRacha, 3);

            // ---- ANIMACIÓN DE LEVITACIÓN ----
            auto floatUp = CCMoveBy::create(1.5f, { 0, 8 });
            auto floatDown = floatUp->reverse();
            auto seq = CCSequence::create(floatUp, floatDown, nullptr);
            auto repeat = CCRepeatForever::create(seq);
            rachaSprite->runAction(repeat);
        }

        // -------- TEXTO DE RACHA DEBAJO DEL SPRITE --------
        auto streakLabel = CCLabelBMFont::create(
            ("Daily streak: " + std::to_string(g_streakData.currentStreak)).c_str(),
            "goldFont.fnt"
        );
        streakLabel->setScale(0.55f);
        streakLabel->setPosition({ winSize.width / 2, centerY - 60 });
        m_mainLayer->addChild(streakLabel);

        // -------- BARRA DE PROGRESO MÁS ABAJO --------
        float barWidth = 140.0f;
        float barHeight = 16.0f;
        float percent = std::min(g_streakData.starsToday / 5.0f, 1.0f);

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

        // -------- ICONO DE ESTRELLA + TEXTO DEL CONTADOR --------
        auto starIcon = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        starIcon->setScale(0.45f);
        starIcon->setPosition({ winSize.width / 2 - 20, centerY - 82 });
        m_mainLayer->addChild(starIcon, 5);

        auto barText = CCLabelBMFont::create(
            (std::to_string(g_streakData.starsToday) + " / 5").c_str(),
            "bigFont.fnt"
        );
        barText->setScale(0.45f);
        barText->setPosition({ winSize.width / 2 + 10, centerY - 82 });
        m_mainLayer->addChild(barText, 5);

        // -------- BOTÓN DE INFORMACIÓN EN LA ESQUINA --------
        auto infoIcon = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        infoIcon->setScale(0.6f);

        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoIcon,
            this,
            menu_selector(InfoPopup::onInfo)
        );

        auto menu = CCMenu::create();
        menu->setPosition({ winSize.width - 20, winSize.height - 20 }); // esquina arriba derecha
        menu->addChild(infoBtn);
        m_mainLayer->addChild(menu, 10);

        return true;
    }

    // ==== CALLBACKS ====
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
        CCSprite* icon;
        if (!spriteName.empty()) {
            icon = CCSprite::create(spriteName.c_str());
            icon->setScale(0.5f);
        }
        else {
            icon = CCSprite::create("btnStreak.png"_spr);
            icon->setScale(0.5f);
        }

        auto circle = CircleButtonSprite::create(
            icon,
            CircleBaseColor::Green,
            CircleBaseSize::Medium
        );

        auto btn = CCMenuItemSpriteExtra::create(
            circle,
            this,
            menu_selector(MyMenuLayer::onOpenPopup)
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
