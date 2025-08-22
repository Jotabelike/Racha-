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
    }
    void save() {
        Mod::get()->setSavedValue<int>("streak", currentStreak);
        Mod::get()->setSavedValue<int>("starsToday", starsToday);
        Mod::get()->setSavedValue<std::string>("lastDay", lastDay);
    }
    void dailyUpdate() {
        time_t t = time(nullptr);
        tm* now = localtime(&t);
        char buf[16];
        strftime(buf, sizeof(buf), "%Y-%m-%d", now);
        std::string today = buf;
        if (lastDay != today) {
            if (starsToday < 5 && lastDay != "") {
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

// =========== POPUP CON BARRA ==============
class InfoPopup : public Popup<> {
protected:
    bool setup() override {
        g_streakData.load();

        this->setTitle("Current streak (Beta)");
        auto winSize = m_mainLayer->getContentSize();

        // Racha actual
        auto streakLabel = CCLabelBMFont::create(
            ("daily streak: " + std::to_string(g_streakData.currentStreak)).c_str(), "goldFont.fnt");
        streakLabel->setScale(0.6f);
        streakLabel->setPosition({ winSize.width / 2, winSize.height / 2 + 38 });
        m_mainLayer->addChild(streakLabel);

        // ------- Barra de progreso --------
        float barWidth = 140.0f;      // angosta
        float barHeight = 16.0f;      // delgada
        float percent = std::min(g_streakData.starsToday / 5.0f, 1.0f);

        // Fondo de la barra gris oscuro
        auto barBg = CCLayerColor::create({ 45, 45, 45, 255 }, barWidth, barHeight);
        barBg->setPosition({ winSize.width / 2 - barWidth / 2, winSize.height / 2 - 18 });
        m_mainLayer->addChild(barBg, 1);

        // Barra de progreso degradado amarillo
        auto barFg = CCLayerGradient::create({ 250, 225, 60, 255 }, { 255, 165, 0, 255 });
        barFg->setContentSize({ barWidth * percent, barHeight });
        barFg->setPosition({ winSize.width / 2 - barWidth / 2, winSize.height / 2 - 18 });
        m_mainLayer->addChild(barFg, 2);

        // Bordes blanco y negro
        auto border = CCLayerColor::create({ 255,255,255,255 }, barWidth + 2, barHeight + 2);
        border->setPosition({ winSize.width / 2 - barWidth / 2 - 1, winSize.height / 2 - 19 });
        border->setZOrder(4);
        border->setOpacity(120);
        m_mainLayer->addChild(border);

        auto outer = CCLayerColor::create({ 0,0,0,255 }, barWidth + 6, barHeight + 6);
        outer->setPosition({ winSize.width / 2 - barWidth / 2 - 3, winSize.height / 2 - 21 });
        outer->setZOrder(0);
        outer->setOpacity(70);
        m_mainLayer->addChild(outer);

        // Texto sobre la barra 
        auto barText = CCLabelBMFont::create(
            (std::to_string(g_streakData.starsToday) + " / 5").c_str(), "bigFont.fnt");
        barText->setScale(0.45f);
        barText->setPosition({ winSize.width / 2, winSize.height / 2 - 11 });
        m_mainLayer->addChild(barText, 5);

        return true;
    }
public:
    static InfoPopup* create() {
        auto ret = new InfoPopup();
        if (ret && ret->initAnchored(260.f, 160.f)) {
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

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return false;

        
        auto icon = CCSprite::create("btnStreak.png"_spr);
        icon->setScale(3.0f); // tamaño (no sirve xd)
        // Crear el botón circular vacío con tu ícono dentro
        auto circle = CircleButtonSprite::create(
            icon,                     
            CircleBaseColor::Green,       // color 
            CircleBaseSize::Medium     // tamaño 
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

