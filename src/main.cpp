﻿#include <Geode/Geode.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJUserScore.hpp>
#include <Geode/binding/GJComment.hpp>
#include <Geode/loader/Log.hpp>
#include <ctime>
#include <string_view>
#include <vector>

using namespace geode::prelude;
using namespace std::literals;
using namespace cocos2d;

// ================== SISTEMA DE RACHAS Y RECOMPENSAS ==================
struct StreakData {
    int currentStreak = 0;
    int starsToday = 0;
    bool hasNewStreak = false;
    std::string lastDay = "";
    std::string equippedBadge = ""; // Nueva variable para la insignia equipada

    // Definir categorías de insignias
    enum class BadgeCategory {
        COMMON,
        SPECIAL,
        EPIC,
        LEGENDARY,
        MYTHIC
    };

    // Sistema flexible de insignias con categorías
    struct BadgeInfo {
        int daysRequired;
        std::string spriteName;
        std::string displayName;
        BadgeCategory category;
        std::string badgeID; // ID único para cada insignia
    };

    std::vector<BadgeInfo> badges = {
        {5, "reward5.png"_spr, "first steps", BadgeCategory::COMMON, "badge_5"},
        {10, "reward10.png"_spr, "Shall we continue?", BadgeCategory::COMMON, "badge_10"},
        {30, "reward30.png"_spr, "We're going well", BadgeCategory::SPECIAL, "badge_30"},
        {50, "reward50.png"_spr, "Half a hundred", BadgeCategory::SPECIAL, "badge_50"},
        {70, "reward70.png"_spr, "Progressing", BadgeCategory::EPIC, "badge_70"},
        {100, "reward100.png"_spr, "100 Days!!!", BadgeCategory::LEGENDARY, "badge_100"},
        {150, "reward150.png"_spr, "150 Days!!!", BadgeCategory::LEGENDARY, "badge_150"},
        {300, "reward300.png"_spr, "300 Days!!!", BadgeCategory::LEGENDARY, "badge_300"},
        {365, "reward1year.png"_spr, "1 year!!!", BadgeCategory::MYTHIC, "badge_365"}
    };

    std::vector<bool> unlockedBadges;

    int getRequiredStars() {
        if (currentStreak >= 81) return 21;
        else if (currentStreak >= 71) return 19;
        else if (currentStreak >= 61) return 17;
        else if (currentStreak >= 51) return 15;
        else if (currentStreak >= 41) return 13;
        else if (currentStreak >= 31) return 11;
        else if (currentStreak >= 21) return 9;
        else if (currentStreak >= 11) return 7;
        else if (currentStreak >= 1) return 5;
        else return 5;
    }

    void load() {
        currentStreak = Mod::get()->getSavedValue<int>("streak", 0);
        starsToday = Mod::get()->getSavedValue<int>("starsToday", 0);
        hasNewStreak = Mod::get()->getSavedValue<bool>("hasNewStreak", false);
        lastDay = Mod::get()->getSavedValue<std::string>("lastDay", "");
        equippedBadge = Mod::get()->getSavedValue<std::string>("equippedBadge", "");

        // Cargar insignias desbloqueadas automáticamente según la configuración
        unlockedBadges.resize(badges.size(), false);
        for (int i = 0; i < badges.size(); i++) {
            unlockedBadges[i] = Mod::get()->getSavedValue<bool>(
                badges[i].badgeID,
                false
            );
        }

        dailyUpdate();
        checkRewards(); // Verificar recompensas al cargar
    }

    void save() {
        Mod::get()->setSavedValue<int>("streak", currentStreak);
        Mod::get()->setSavedValue<int>("starsToday", starsToday);
        Mod::get()->setSavedValue<bool>("hasNewStreak", hasNewStreak);
        Mod::get()->setSavedValue<std::string>("lastDay", lastDay);
        Mod::get()->setSavedValue<std::string>("equippedBadge", equippedBadge);

        // Guardar insignias desbloqueadas
        for (int i = 0; i < badges.size(); i++) {
            Mod::get()->setSavedValue<bool>(
                badges[i].badgeID,
                unlockedBadges[i]
            );
        }
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

    void checkRewards() {
        for (int i = 0; i < badges.size(); i++) {
            if (currentStreak >= badges[i].daysRequired && !unlockedBadges[i]) {
                unlockedBadges[i] = true;
            }
        }
        save();
    }

    void addStars(int count) {
        load();
        dailyUpdate();

        // Obtener los requisitos ANTES de cualquier cambio
        int currentRequired = getRequiredStars();
        bool alreadyGotRacha = (starsToday >= currentRequired);
        starsToday += count;

        if (!alreadyGotRacha && starsToday >= currentRequired) {
            // Guardar el requerimiento actual antes de incrementar
            int oldRequired = currentRequired;

            currentStreak++;
            hasNewStreak = true;

            // Obtener el NUEVO requerimiento después de incrementar la racha
            int newRequired = getRequiredStars();

            // Si los requisitos aumentan (cambio de categoría de racha)
            if (newRequired > oldRequired) {
                // Mantener el progreso del día anterior completado
                starsToday = oldRequired;
            }

            checkRewards();
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

    // Función para añadir una nueva insignia fácilmente con categoría
    void addBadge(int days, const std::string& sprite, const std::string& name, BadgeCategory category, const std::string& id) {
        badges.push_back({ days, sprite, name, category, id });
        unlockedBadges.push_back(false);
    }

    // Función para obtener el nombre de la categoría
    std::string getCategoryName(BadgeCategory category) {
        switch (category) {
        case BadgeCategory::COMMON: return "Common";
        case BadgeCategory::SPECIAL: return "Special";
        case BadgeCategory::EPIC: return "Epic";
        case BadgeCategory::LEGENDARY: return "Legendary";
        case BadgeCategory::MYTHIC: return "Mythic";
        default: return "Unknown";
        }
    }

    // Función para obtener el color de la categoría
    ccColor3B getCategoryColor(BadgeCategory category) {
        switch (category) {
        case BadgeCategory::COMMON: return ccc3(200, 200, 200); // Gris
        case BadgeCategory::SPECIAL: return ccc3(100, 200, 255); // Azul claro
        case BadgeCategory::EPIC: return ccc3(170, 0, 255); // Púrpura
        case BadgeCategory::LEGENDARY: return ccc3(255, 165, 0); // Naranja
        case BadgeCategory::MYTHIC: return ccc3(255, 50, 50); // Rojo
        default: return ccc3(255, 255, 255);
        }
    }

    // Obtener información de una insignia por su ID
    BadgeInfo* getBadgeInfo(const std::string& badgeID) {
        for (auto& badge : badges) {
            if (badge.badgeID == badgeID) {
                return &badge;
            }
        }
        return nullptr;
    }

    // Verificar si una insignia está desbloqueada
    bool isBadgeUnlocked(const std::string& badgeID) {
        for (int i = 0; i < badges.size(); i++) {
            if (badges[i].badgeID == badgeID) {
                return unlockedBadges[i];
            }
        }
        return false;
    }

    // Equipar una insignia
    void equipBadge(const std::string& badgeID) {
        if (isBadgeUnlocked(badgeID)) {
            equippedBadge = badgeID;
            save();
        }
    }

    // Desequipar la insignia actual
    void unequipBadge() {
        equippedBadge = "";
        save();
    }

    // Obtener la insignia equipada actualmente
    BadgeInfo* getEquippedBadge() {
        if (equippedBadge.empty()) return nullptr;
        return getBadgeInfo(equippedBadge);
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

// ============= POPUP PARA EQUIPAR INSIGNIAS =============
class EquipBadgePopup : public Popup<std::string> {
protected:
    std::string m_badgeID;

    bool setup(std::string badgeID) override {
        m_badgeID = badgeID;
        auto winSize = m_mainLayer->getContentSize();

        auto badgeInfo = g_streakData.getBadgeInfo(badgeID);
        if (!badgeInfo) return false;

        this->setTitle("Equip Badge");

        // Mostrar la insignia
        auto badgeSprite = CCSprite::create(badgeInfo->spriteName.c_str());
        if (badgeSprite) {
            badgeSprite->setScale(0.3f);
            badgeSprite->setPosition(ccp(winSize.width / 2, winSize.height / 2 + 20));
            m_mainLayer->addChild(badgeSprite);
        }

        // Mostrar nombre de la insignia
        auto nameLabel = CCLabelBMFont::create(badgeInfo->displayName.c_str(), "goldFont.fnt");
        nameLabel->setScale(0.6f);
        nameLabel->setPosition(ccp(winSize.width / 2, winSize.height / 2 - 20));
        m_mainLayer->addChild(nameLabel);

        // Botón para equipar
        auto equipBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Equip"),
            this,
            menu_selector(EquipBadgePopup::onEquip)
        );
        equipBtn->setPosition(ccp(winSize.width / 2, winSize.height / 2 - 60));

        auto menu = CCMenu::create();
        menu->addChild(equipBtn);
        menu->setPosition(0, 0);
        m_mainLayer->addChild(menu);

        return true;
    }

    void onEquip(CCObject*) {
        g_streakData.equipBadge(m_badgeID);
        FLAlertLayer::create("Success", "Badge equipped!", "OK")->show();
        this->onClose(nullptr);
    }

public:
    static EquipBadgePopup* create(std::string badgeID) {
        auto ret = new EquipBadgePopup();
        if (ret && ret->initAnchored(200.f, 150.f, badgeID)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ============= POPUP QUE MUESTRA TODAS LAS RACHAS =============
class AllRachasPopup : public Popup<> {
protected:
    bool setup() override {
        this->setTitle("All Streaks");
        auto winSize = m_mainLayer->getContentSize();

        float startX = 50.f;
        float y = winSize.height / 2 + 20.f;
        float spacing = 50.f;

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
            spr->setScale(0.22f);
            spr->setPosition(ccp(startX + i * spacing, y));
            m_mainLayer->addChild(spr);

            auto label = CCLabelBMFont::create(
                CCString::createWithFormat("Day %d", day)->getCString(),
                "goldFont.fnt"
            );
            label->setScale(0.35f);
            label->setPosition(ccp(startX + i * spacing, y - 40));
            m_mainLayer->addChild(label);

            auto starIcon = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
            starIcon->setScale(0.4f);
            starIcon->setPosition(ccp(startX + i * spacing - 4, y - 60));
            m_mainLayer->addChild(starIcon);

            auto starsLabel = CCLabelBMFont::create(
                CCString::createWithFormat("%d", requiredStars)->getCString(),
                "bigFont.fnt"
            );
            starsLabel->setScale(0.3f);
            starsLabel->setPosition(ccp(startX + i * spacing + 6, y - 60));
            m_mainLayer->addChild(starsLabel);

            i++;
        }

        return true;
    }

public:
    static AllRachasPopup* create() {
        auto ret = new AllRachasPopup();
        if (ret && ret->initAnchored(500.f, 155.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ============= POPUP DE PROGRESO MÚLTIPLE =============
class DayProgressPopup : public Popup<> {
protected:
    int m_currentGoalIndex = 0;
    CCLabelBMFont* m_titleLabel = nullptr;
    CCLayerColor* m_barBg = nullptr;
    CCLayerGradient* m_barFg = nullptr;
    CCLayerColor* m_border = nullptr;
    CCLayerColor* m_outer = nullptr;
    CCSprite* m_rewardSprite = nullptr;
    CCLabelBMFont* m_dayText = nullptr;

    bool setup() override {
        auto winSize = m_mainLayer->getContentSize();

        // Botón izquierdo
        auto leftArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        leftArrow->setScale(0.8f);
        auto leftBtn = CCMenuItemSpriteExtra::create(
            leftArrow, this, menu_selector(DayProgressPopup::onPreviousGoal)
        );
        leftBtn->setPosition(ccp(-winSize.width / 2 + 30, 0));

        // Botón derecho
        auto rightArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        rightArrow->setScale(0.8f);
        rightArrow->setFlipX(true);
        auto rightBtn = CCMenuItemSpriteExtra::create(
            rightArrow, this, menu_selector(DayProgressPopup::onNextGoal)
        );
        rightBtn->setPosition(ccp(winSize.width / 2 - 30, 0));

        auto arrowMenu = CCMenu::create();
        arrowMenu->addChild(leftBtn);
        arrowMenu->addChild(rightBtn);
        arrowMenu->setPosition(ccp(winSize.width / 2, winSize.height / 2));
        m_mainLayer->addChild(arrowMenu);

        // Crear elementos de la barra
        m_barBg = CCLayerColor::create(ccc4(45, 45, 45, 255), 180.0f, 16.0f);
        m_barFg = CCLayerGradient::create(ccc4(250, 225, 60, 255), ccc4(255, 165, 0, 255));
        m_border = CCLayerColor::create(ccc4(255, 255, 255, 255), 182.0f, 18.0f);
        m_outer = CCLayerColor::create(ccc4(0, 0, 0, 255), 186.0f, 22.0f);

        m_barBg->setVisible(false);
        m_barFg->setVisible(false);
        m_border->setVisible(false);
        m_outer->setVisible(false);

        m_mainLayer->addChild(m_barBg, 1);
        m_mainLayer->addChild(m_barFg, 2);
        m_mainLayer->addChild(m_border, 4);
        m_mainLayer->addChild(m_outer, 0);

        m_titleLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_titleLabel->setScale(0.6f);
        m_titleLabel->setPosition(ccp(winSize.width / 2, winSize.height / 2 + 60));
        m_mainLayer->addChild(m_titleLabel, 5);

        m_dayText = CCLabelBMFont::create("", "goldFont.fnt");
        m_dayText->setScale(0.5f);
        m_dayText->setPosition(ccp(winSize.width / 2, winSize.height / 2 - 35));
        m_mainLayer->addChild(m_dayText, 5);

        updateDisplay();

        return true;
    }

    void updateDisplay() {
        auto winSize = m_mainLayer->getContentSize();
        g_streakData.load();

        // Usar el sistema automático de insignias
        if (m_currentGoalIndex >= g_streakData.badges.size()) {
            m_currentGoalIndex = 0;
        }

        auto& badge = g_streakData.badges[m_currentGoalIndex];
        int currentDays = std::max(0, std::min(g_streakData.currentStreak, badge.daysRequired));
        float percent = currentDays / static_cast<float>(badge.daysRequired);

        m_titleLabel->setString(CCString::createWithFormat("Progress to %d Days", badge.daysRequired)->getCString());

        // Barra de progreso
        float barWidth = 180.0f;
        float barHeight = 16.0f;

        m_barBg->setContentSize(CCSize(barWidth, barHeight));
        m_barBg->setPosition(ccp(winSize.width / 2 - barWidth / 2, winSize.height / 2 - 10));
        m_barBg->setVisible(true);

        m_barFg->setContentSize(CCSize(barWidth * percent, barHeight));
        m_barFg->setPosition(ccp(winSize.width / 2 - barWidth / 2, winSize.height / 2 - 10));
        m_barFg->setVisible(true);

        m_border->setContentSize(CCSize(barWidth + 2, barHeight + 2));
        m_border->setPosition(ccp(winSize.width / 2 - barWidth / 2 - 1, winSize.height / 2 - 11));
        m_border->setVisible(true);
        m_border->setOpacity(120);

        m_outer->setContentSize(CCSize(barWidth + 6, barHeight + 6));
        m_outer->setPosition(ccp(winSize.width / 2 - barWidth / 2 - 3, winSize.height / 2 - 13));
        m_outer->setVisible(true);
        m_outer->setOpacity(70);

        // Eliminar sprite de recompensa anterior
        if (m_rewardSprite) {
            m_rewardSprite->removeFromParent();
            m_rewardSprite = nullptr;
        }

        // Insignia centrada encima de la barra
        m_rewardSprite = CCSprite::create(badge.spriteName.c_str());
        if (m_rewardSprite) {
            m_rewardSprite->setScale(0.25f);
            m_rewardSprite->setPosition(ccp(
                winSize.width / 2,
                winSize.height / 2 + 30
            ));
            m_mainLayer->addChild(m_rewardSprite, 5);
        }

        // Texto de días
        m_dayText->setString(
            CCString::createWithFormat("Day %d / %d", currentDays, badge.daysRequired)->getCString()
        );
    }

    void onNextGoal(CCObject*) {
        m_currentGoalIndex = (m_currentGoalIndex + 1) % g_streakData.badges.size();
        updateDisplay();
    }

    void onPreviousGoal(CCObject*) {
        m_currentGoalIndex = (m_currentGoalIndex - 1 + g_streakData.badges.size()) % g_streakData.badges.size();
        updateDisplay();
    }

public:
    static DayProgressPopup* create() {
        auto ret = new DayProgressPopup();
        if (ret && ret->initAnchored(300.f, 180.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ============= POPUP DE RECOMPENSAS (INSIGNIAS) CON CATEGORÍAS =============
class RewardsPopup : public Popup<> {
protected:
    int m_currentCategory = 0;
    CCLabelBMFont* m_categoryLabel = nullptr;
    CCLayerColor* m_darkPanel = nullptr;
    CCNode* m_badgeContainer = nullptr;
    int m_colorIndex = 0;
    float m_colorTransitionTime = 0.0f;
    std::vector<ccColor3B> m_mythicColors;
    ccColor3B m_currentColor;
    ccColor3B m_targetColor;

    bool setup() override {
        this->setTitle("Awards Collection");
        auto winSize = m_mainLayer->getContentSize();

        g_streakData.load();

        // Inicializar colores para Mythic
        m_mythicColors = {
            ccc3(255, 0, 0),      // Rojo
            ccc3(255, 165, 0),    // Naranja
            ccc3(255, 255, 0),    // Amarillo
            ccc3(0, 255, 0),      // Verde
            ccc3(0, 0, 255),      // Azul
            ccc3(75, 0, 130),     // Índigo
            ccc3(238, 130, 238),  // Violeta
            ccc3(255, 105, 180),  // Rosa
            ccc3(255, 215, 0),    // Oro
            ccc3(192, 192, 192)   // Plata
        };
        m_colorIndex = 0;
        m_currentColor = m_mythicColors[0];
        m_targetColor = m_mythicColors[1];
        m_colorTransitionTime = 0.0f;

        // Crear panel oscuro para mejor contraste
        m_darkPanel = CCLayerColor::create(ccc4(30, 30, 30, 200), winSize.width - 40, 140);
        m_darkPanel->setPosition(ccp(20, winSize.height / 2 - 50));
        m_darkPanel->setZOrder(-1);
        m_mainLayer->addChild(m_darkPanel);

        // Botón izquierdo
        auto leftArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        leftArrow->setScale(0.8f);
        auto leftBtn = CCMenuItemSpriteExtra::create(
            leftArrow, this, menu_selector(RewardsPopup::onPreviousCategory)
        );
        leftBtn->setPosition(ccp(-winSize.width / 2 + 30, 60));

        // Botón derecho
        auto rightArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        rightArrow->setScale(0.8f);
        rightArrow->setFlipX(true);
        auto rightBtn = CCMenuItemSpriteExtra::create(
            rightArrow, this, menu_selector(RewardsPopup::onNextCategory)
        );
        rightBtn->setPosition(ccp(winSize.width / 2 - 30, 60));

        auto arrowMenu = CCMenu::create();
        arrowMenu->addChild(leftBtn);
        arrowMenu->addChild(rightBtn);
        arrowMenu->setPosition(ccp(winSize.width / 2, winSize.height / 2));
        m_mainLayer->addChild(arrowMenu);

        // Etiqueta de categoría
        m_categoryLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_categoryLabel->setScale(0.6f);
        m_categoryLabel->setPosition(ccp(winSize.width / 2, winSize.height / 2 + 60));
        m_mainLayer->addChild(m_categoryLabel);

        // Contenedor para insignias
        m_badgeContainer = CCNode::create();
        m_badgeContainer->setPosition(0, 0);
        m_mainLayer->addChild(m_badgeContainer);

        // Mostrar la categoría actual
        updateCategoryDisplay();

        // Programar actualización de colores
        this->schedule(schedule_selector(RewardsPopup::updateColorEffect), 0.016f); // ~60 FPS

        return true;
    }

    void updateColorEffect(float dt) {
        StreakData::BadgeCategory currentCat = static_cast<StreakData::BadgeCategory>(m_currentCategory);

        if (currentCat == StreakData::BadgeCategory::MYTHIC && m_categoryLabel) {
            m_colorTransitionTime += dt;

            // Transición suave entre colores (3 segundos por transición)
            float transitionDuration = 1.0f;

            if (m_colorTransitionTime >= transitionDuration) {
                m_colorTransitionTime = 0.0f;
                m_colorIndex = (m_colorIndex + 1) % m_mythicColors.size();
                m_currentColor = m_targetColor;
                m_targetColor = m_mythicColors[(m_colorIndex + 1) % m_mythicColors.size()];
            }

            // Interpolación lineal entre colores
            float progress = m_colorTransitionTime / transitionDuration;
            ccColor3B interpolatedColor = ccc3(
                m_currentColor.r + (m_targetColor.r - m_currentColor.r) * progress,
                m_currentColor.g + (m_targetColor.g - m_currentColor.g) * progress,
                m_currentColor.b + (m_targetColor.b - m_currentColor.b) * progress
            );

            m_categoryLabel->setColor(interpolatedColor);
        }
    }

    void updateCategoryDisplay() {
        // Limpiar contenedor de insignias
        m_badgeContainer->removeAllChildren();

        // Obtener todas las insignias de la categoría actual
        StreakData::BadgeCategory currentCat = static_cast<StreakData::BadgeCategory>(m_currentCategory);
        std::vector<StreakData::BadgeInfo> categoryBadges;

        for (auto& badge : g_streakData.badges) {
            if (badge.category == currentCat) {
                categoryBadges.push_back(badge);
            }
        }

        // Actualizar etiqueta de categoría
        std::string categoryName = g_streakData.getCategoryName(currentCat);
        m_categoryLabel->setString(categoryName.c_str());

        // Detener cualquier animación previa y resetear escala
        m_categoryLabel->stopAllActions();
        m_categoryLabel->setScale(0.6f);

        // Reiniciar transición de colores para Mythic
        if (currentCat == StreakData::BadgeCategory::MYTHIC) {
            m_colorIndex = 0;
            m_colorTransitionTime = 0.0f;
            m_currentColor = m_mythicColors[0];
            m_targetColor = m_mythicColors[1];
            m_categoryLabel->setColor(m_currentColor);
        }
        else {
            // Color normal para otras categorías
            m_categoryLabel->setColor(g_streakData.getCategoryColor(currentCat));
        }

        // Posicionar insignias
        auto winSize = m_mainLayer->getContentSize();
        float startX = winSize.width / 2 - (categoryBadges.size() * 45.f / 2) + 22.5f;
        float y = winSize.height / 2 + 20;
        float spacing = 45.f;

        for (int i = 0; i < categoryBadges.size(); i++) {
            auto& badge = categoryBadges[i];
            bool unlocked = g_streakData.isBadgeUnlocked(badge.badgeID);
            bool equipped = (g_streakData.getEquippedBadge() && g_streakData.getEquippedBadge()->badgeID == badge.badgeID);

            // Sprite de la insignia
            auto badgeSprite = CCSprite::create(badge.spriteName.c_str());
            if (badgeSprite) {
                badgeSprite->setScale(0.25f);
                badgeSprite->setPosition(ccp(startX + i * spacing, y));

                if (!unlocked) {
                    badgeSprite->setColor(ccc3(100, 100, 100));
                }
                else if (equipped) {
                    // Resaltar insignia equipada
                    auto glow = CCSprite::createWithSpriteFrameName("GJ_glow_01.png");
                    glow->setScale(0.5f);
                    glow->setPosition(ccp(badgeSprite->getContentSize().width / 2, badgeSprite->getContentSize().height / 2));
                    glow->setColor(g_streakData.getCategoryColor(currentCat));
                    glow->setOpacity(150);
                    badgeSprite->addChild(glow);
                }

                // Hacer la insignia clickeable si está desbloqueada
                if (unlocked) {
                    auto badgeBtn = CCMenuItemSpriteExtra::create(
                        badgeSprite,
                        this,
                        menu_selector(RewardsPopup::onBadgeClick)
                    );
                    badgeBtn->setTag(i); // Guardar índice para identificarla
                    badgeBtn->setUserObject(CCString::create(badge.badgeID));

                    auto badgeMenu = CCMenu::create();
                    badgeMenu->addChild(badgeBtn);
                    badgeMenu->setPosition(ccp(startX + i * spacing, y));
                    m_badgeContainer->addChild(badgeMenu);
                }
                else {
                    m_badgeContainer->addChild(badgeSprite);
                }
            }

            // Texto de días requeridos
            auto daysLabel = CCLabelBMFont::create(
                CCString::createWithFormat("%d days", badge.daysRequired)->getCString(),
                "goldFont.fnt"
            );
            daysLabel->setScale(0.3f);
            daysLabel->setPosition(ccp(startX + i * spacing, y - 35));

            if (!unlocked) {
                daysLabel->setColor(ccc3(150, 150, 150));
            }
            else {
                daysLabel->setColor(g_streakData.getCategoryColor(currentCat));
            }

            m_badgeContainer->addChild(daysLabel);

            // Icono de candado para no desbloqueadas
            if (!unlocked) {
                auto lockIcon = CCSprite::createWithSpriteFrameName("GJ_lock_001.png");
                lockIcon->setScale(0.4f);
                lockIcon->setPosition(ccp(startX + i * spacing, y));
                m_badgeContainer->addChild(lockIcon, 5);
            }
        }

        // Contador de insignias para esta categoría
        int unlockedCount = 0;
        int totalInCategory = categoryBadges.size();

        for (auto& badge : categoryBadges) {
            if (g_streakData.isBadgeUnlocked(badge.badgeID)) {
                unlockedCount++;
            }
        }

        auto counterText = CCLabelBMFont::create(
            CCString::createWithFormat("Unlocked: %d/%d", unlockedCount, totalInCategory)->getCString(),
            "bigFont.fnt"
        );
        counterText->setScale(0.4f);
        counterText->setPosition(ccp(winSize.width / 2, winSize.height / 2 - 60));
        m_badgeContainer->addChild(counterText);
    }
    void onBadgeClick(CCObject* sender) {
        auto menuItem = static_cast<CCMenuItemSpriteExtra*>(sender);
        auto badgeID = static_cast<CCString*>(menuItem->getUserObject())->getCString();

        // Mostrar popup para equipar la insignia
        EquipBadgePopup::create(badgeID)->show();
    }

    void onNextCategory(CCObject*) {
        // Reiniciar para la nueva categoría
        m_currentCategory = (m_currentCategory + 1) % 5;
        updateCategoryDisplay();
    }

    void onPreviousCategory(CCObject*) {
        // Reiniciar para la nueva categoría
        m_currentCategory = (m_currentCategory - 1 + 5) % 5;
        updateCategoryDisplay();
    }

public:
    static RewardsPopup* create() {
        auto ret = new RewardsPopup();
        // Tamaño fijo para el popup
        if (ret && ret->initAnchored(300.f, 200.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    // Limpiar al cerrar el popup
    void onClose(CCObject* sender) override {
        this->unschedule(schedule_selector(RewardsPopup::updateColorEffect));
        Popup::onClose(sender);
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
        menuRacha->setPosition(ccp(winSize.width / 2, centerY));
        m_mainLayer->addChild(menuRacha, 3);

        // Animación de levitación
        auto floatUp = CCMoveBy::create(1.5f, ccp(0, 8));
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
        streakLabel->setPosition(ccp(winSize.width / 2, centerY - 60));
        m_mainLayer->addChild(streakLabel);

        // Barra de progreso (diseño original)
        float barWidth = 140.0f;
        float barHeight = 16.0f;
        int requiredStars = g_streakData.getRequiredStars();
        float percent = std::min(g_streakData.starsToday / static_cast<float>(requiredStars), 1.0f);

        auto barBg = CCLayerColor::create(ccc4(45, 45, 45, 255), barWidth, barHeight);
        barBg->setPosition(ccp(winSize.width / 2 - barWidth / 2, centerY - 90));
        m_mainLayer->addChild(barBg, 1);

        auto barFg = CCLayerGradient::create(ccc4(250, 225, 60, 255), ccc4(255, 165, 0, 255));
        barFg->setContentSize(CCSize(barWidth * percent, barHeight));
        barFg->setPosition(ccp(winSize.width / 2 - barWidth / 2, centerY - 90));
        m_mainLayer->addChild(barFg, 2);

        auto border = CCLayerColor::create(ccc4(255, 255, 255, 255), barWidth + 2, barHeight + 2);
        border->setPosition(ccp(winSize.width / 2 - barWidth / 2 - 1, centerY - 91));
        border->setZOrder(4);
        border->setOpacity(120);
        m_mainLayer->addChild(border);

        auto outer = CCLayerColor::create(ccc4(0, 0, 0, 255), barWidth + 6, barHeight + 6);
        outer->setPosition(ccp(winSize.width / 2 - barWidth / 2 - 3, centerY - 93));
        outer->setZOrder(0);
        outer->setOpacity(70);
        m_mainLayer->addChild(outer);

        // Icono de estrella
        auto starIcon = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        starIcon->setScale(0.45f);
        starIcon->setPosition(ccp(winSize.width / 2 - 25, centerY - 82));
        m_mainLayer->addChild(starIcon, 5);

        // Texto del contador de estrellas
        auto barText = CCLabelBMFont::create(
            (std::to_string(g_streakData.starsToday) + " / " + std::to_string(requiredStars)).c_str(),
            "bigFont.fnt"
        );
        barText->setScale(0.45f);
        barText->setPosition(ccp(winSize.width / 2 + 15, centerY - 82));
        m_mainLayer->addChild(barText, 5);

        // INDICADOR DE RACHA
        std::string indicatorSpriteName;
        if (g_streakData.starsToday >= requiredStars) {
            indicatorSpriteName = g_streakData.getRachaSprite();
        }
        else {
            indicatorSpriteName = "racha0.png"_spr;
        }

        auto rachaIndicator = CCSprite::create(indicatorSpriteName.c_str());
        rachaIndicator->setScale(0.14f);
        rachaIndicator->setPosition(ccp(winSize.width / 2 + barWidth / 2 + 20, centerY - 82));
        m_mainLayer->addChild(rachaIndicator, 5);

        // ===== BOTÓN STATS =====
        auto statsIcon = CCSprite::create("BtnStats.png"_spr);
        if (statsIcon) {
            statsIcon->setScale(0.7f);
            auto statsBtn = CCMenuItemSpriteExtra::create(
                statsIcon, this, menu_selector(InfoPopup::onOpenStats)
            );
            auto statsMenu = CCMenu::create();
            statsMenu->addChild(statsBtn);
            statsMenu->setPosition(ccp(winSize.width - 22, centerY));
            m_mainLayer->addChild(statsMenu, 10);
        }

        // ===== BOTÓN DE RECOMPENSAS =====
        auto rewardsIcon = CCSprite::create("RewardsBtn.png"_spr);
        if (rewardsIcon) {
            rewardsIcon->setScale(0.7f);
            auto rewardsBtn = CCMenuItemSpriteExtra::create(
                rewardsIcon, this, menu_selector(InfoPopup::onOpenRewards)
            );
            auto rewardsMenu = CCMenu::create();
            rewardsMenu->addChild(rewardsBtn);
            rewardsMenu->setPosition(ccp(winSize.width - 22, centerY - 37));
            m_mainLayer->addChild(rewardsMenu, 10);
        }

        // Botón de información
        auto infoIcon = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        infoIcon->setScale(0.6f);

        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoIcon, this, menu_selector(InfoPopup::onInfo)
        );

        auto menu = CCMenu::create();
        menu->setPosition(ccp(winSize.width - 20, winSize.height - 20));
        menu->addChild(infoBtn);
        m_mainLayer->addChild(menu, 10);

        // Mostrar animación si hay nueva racha (sin alertas)
        if (g_streakData.shouldShowAnimation()) {
            this->showStreakAnimation(g_streakData.currentStreak);
        }

        return true;
    }

    void onOpenStats(CCObject*) {
        DayProgressPopup::create()->show();
    }

    void onOpenRewards(CCObject*) {
        RewardsPopup::create()->show();
    }

    void onInfo(CCObject*) {
        FLAlertLayer::create(
            "About Streak!",
            "Collect 5+ stars every day to increase your streak!\n"
            "If you miss a day (less than required), your streak resets.\n\n"
            "Icons change depending on how many days you keep your streak.\n"
            "Earn special awards for maintaining your streak!",
            "OK"
        )->show();
    }

    void onRachaClick(CCObject*) {
        AllRachasPopup::create()->show();
    }

    void showStreakAnimation(int streakLevel) {
        auto winSize = this->getContentSize();

        auto animLayer = CCLayer::create();
        animLayer->setContentSize(winSize);
        animLayer->setPosition(0, 0);
        animLayer->setZOrder(1000);
        this->addChild(animLayer);

        auto bg = CCLayerColor::create(ccc4(0, 0, 0, 180), winSize.width, winSize.height);
        bg->setPosition(0, 0);
        animLayer->addChild(bg);

        std::string spriteName = g_streakData.getRachaSprite();

        auto rachaSprite = CCSprite::create(spriteName.c_str());
        rachaSprite->setPosition(ccp(winSize.width / 2, winSize.height / 2));
        rachaSprite->setScale(0.1f);
        animLayer->addChild(rachaSprite);

        auto newStreakSprite = CCSprite::create("NewStreak.png"_spr);
        if (!newStreakSprite) {
            newStreakSprite = CCSprite::create();
            auto fallbackText = CCLabelBMFont::create("New Streak!", "bigFont.fnt");
            fallbackText->setColor(ccc3(255, 255, 0));
            newStreakSprite->addChild(fallbackText);
        }
        newStreakSprite->setPosition(ccp(winSize.width / 2, winSize.height / 2 + 100));
        newStreakSprite->setScale(0.1f);
        animLayer->addChild(newStreakSprite);

        auto daysLabel = CCLabelBMFont::create(
            CCString::createWithFormat("Day %d", streakLevel)->getCString(),
            "goldFont.fnt"
        );
        daysLabel->setPosition(ccp(winSize.width / 2, winSize.height / 2 - 80));
        daysLabel->setScale(0.8f);
        daysLabel->setColor(ccc3(255, 215, 0));
        animLayer->addChild(daysLabel);

        auto scaleUp = CCScaleTo::create(0.8f, 1.2f);
        auto scaleDown = CCScaleTo::create(0.3f, 1.0f);
        auto scaleSequence = CCSequence::create(scaleUp, scaleDown, nullptr);
        auto repeat = CCRepeatForever::create(scaleSequence);
        rachaSprite->runAction(repeat);

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

        daysLabel->runAction(
            CCRepeatForever::create(
                CCSequence::create(
                    CCFadeTo::create(0.5f, 150),
                    CCFadeTo::create(0.5f, 255),
                    nullptr
                )
            )
        );

        for (int i = 0; i < 3; i++) {
            auto glow = CCSprite::create();
            glow->setTextureRect(CCRect(0, 0, 200, 200));
            glow->setColor(ccc3(255, 255, 100));
            glow->setPosition(ccp(winSize.width / 2, winSize.height / 2));
            glow->setScale(0.5f + i * 0.3f);
            glow->setOpacity(100 - i * 30);
            glow->setBlendFunc(ccBlendFunc{ GL_SRC_ALPHA, GL_ONE });
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

        CCArray* children = animLayer->getChildren();
        for (int i = 0; i < children->count(); i++) {
            auto child = dynamic_cast<CCNode*>(children->objectAtIndex(i));
            if (child) {
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

        animLayer->runAction(
            CCSequence::create(
                CCDelayTime::create(4.5f),
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

        // Crear el botón principal
        auto circle = CircleButtonSprite::create(
            icon, CircleBaseColor::Green, CircleBaseSize::Medium
        );

        // Añadir exclamación si la racha está inactiva
        int requiredStars = g_streakData.getRequiredStars();
        bool streakInactive = (g_streakData.starsToday < requiredStars);

        if (streakInactive) {
            auto alertSprite = CCSprite::createWithSpriteFrameName("exMark_001.png");
            if (alertSprite) {
                alertSprite->setScale(0.4f);
                alertSprite->setPosition(ccp(circle->getContentSize().width - 12, circle->getContentSize().height - 12));
                alertSprite->setZOrder(10);
                circle->addChild(alertSprite);

                // Efecto de parpadeo opcional
                auto blink = CCBlink::create(2.0f, 3);
                auto repeat = CCRepeatForever::create(blink);
                alertSprite->runAction(repeat);
            }
        }

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

// ============= MODIFICACIONES PARA MOSTRAR INSIGNIAS EN PERFIL =============

class $modify(ProfilePage) {
    struct Fields {
        CCSprite* badgeSprite = nullptr;
    };

    void loadPageFromUserInfo(GJUserScore * a2) {
        ProfilePage::loadPageFromUserInfo(a2);

        // Solo mostrar en el perfil propio
        if (a2->m_accountID == GJAccountManager::get()->m_accountID) {
            auto layer = m_mainLayer;
            CCMenu* username_menu = static_cast<CCMenu*>(layer->getChildByIDRecursive("username-menu"));

            if (username_menu) {
                // Eliminar insignia anterior si existe
                if (m_fields->badgeSprite) {
                    m_fields->badgeSprite->removeFromParent();
                    m_fields->badgeSprite = nullptr;
                }

                // Obtener insignia equipada
                auto equippedBadge = g_streakData.getEquippedBadge();
                if (equippedBadge) {
                    // Crear y mostrar sprite de la insignia
                    m_fields->badgeSprite = CCSprite::create(equippedBadge->spriteName.c_str());
                    if (m_fields->badgeSprite) {
                        m_fields->badgeSprite->setScale(0.2f);
                        m_fields->badgeSprite->setID("streak-badge");

                        // Añadir animación de levitación
                        auto floatUp = CCMoveBy::create(1.5f, ccp(0, 8));
                        auto floatDown = floatUp->reverse();
                        auto sequence = CCSequence::create(floatUp, floatDown, nullptr);
                        auto repeat = CCRepeatForever::create(sequence);
                        m_fields->badgeSprite->runAction(repeat);

                        // Posicionar la insignia al lado del nombre de usuario
                        username_menu->addChild(m_fields->badgeSprite);
                        username_menu->updateLayout();
                    }
                }
            }
        }
    }
};

class $modify(CommentCell) {
    struct Fields {
        CCSprite* badgeSprite = nullptr;
    };

    void loadFromComment(GJComment * p0) {
        CommentCell::loadFromComment(p0);

        // Solo mostrar en comentarios del usuario actual
        if (p0->m_userID == GJAccountManager::get()->m_accountID) {
            auto layer = m_mainLayer;
            CCMenu* username_menu = static_cast<CCMenu*>(layer->getChildByIDRecursive("username-menu"));

            if (username_menu) {
                // Eliminar insignia anterior si existe
                if (m_fields->badgeSprite) {
                    m_fields->badgeSprite->removeFromParent();
                    m_fields->badgeSprite = nullptr;
                }

                // Obtener insignia equipada
                auto equippedBadge = g_streakData.getEquippedBadge();
                if (equippedBadge) {
                    // Crear y mostrar sprite de la insignia
                    m_fields->badgeSprite = CCSprite::create(equippedBadge->spriteName.c_str());
                    if (m_fields->badgeSprite) {
                        m_fields->badgeSprite->setScale(0.15f);
                        m_fields->badgeSprite->setID("streak-badge");

                        // Añadir animación de levitación
                        auto floatUp = CCMoveBy::create(1.5f, ccp(0, 6));
                        auto floatDown = floatUp->reverse();
                        auto sequence = CCSequence::create(floatUp, floatDown, nullptr);
                        auto repeat = CCRepeatForever::create(sequence);
                        m_fields->badgeSprite->runAction(repeat);

                        // Posicionar la insignia al lado del nombre de usuario
                        username_menu->addChild(m_fields->badgeSprite);
                        username_menu->updateLayout();
                    }
                }
            }
        }
    }
};