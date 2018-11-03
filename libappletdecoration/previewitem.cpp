﻿/*
 * Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
 *           2018  Michail Vourlakos <mvourlakos@gmail.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "previewitem.h"
#include "previewbridge.h"
#include "previewsettings.h"
#include "previewclient.h"
#include <KDecoration2/Decoration>
#include <KDecoration2/DecorationSettings>
#include <KDecoration2/DecorationShadow>
#include <KDecoration2/DecoratedClient>
#include <QCoreApplication>
#include <QCursor>
#include <QPainter>
#include <QQmlContext>
#include <QQmlEngine>

#include <cmath>

#include <QDebug>

namespace Decoration {
namespace Applet {

PreviewItem::PreviewItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_decoration(nullptr)
    , m_windowColor(QPalette().background().color())
{
    setAcceptHoverEvents(true);
    setFiltersChildMouseEvents(true);
    setAcceptedMouseButtons(Qt::MouseButtons(~Qt::NoButton));
    connect(this, &PreviewItem::widthChanged, this, &PreviewItem::syncSize);
    connect(this, &PreviewItem::heightChanged, this, &PreviewItem::syncSize);
    connect(this, &PreviewItem::bridgeChanged, this, &PreviewItem::createDecoration);
    connect(this, &PreviewItem::settingsChanged, this, &PreviewItem::createDecoration);
}

PreviewItem::~PreviewItem()
{
    m_decoration->deleteLater();

    if (m_bridge) {
        m_bridge->unregisterPreviewItem(this);
    }
}

void PreviewItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    createDecoration();

    if (m_decoration) {
        m_decoration->setSettings(m_settings->settings());
        m_decoration->init();
        syncSize();
    }
}

void PreviewItem::createDecoration()
{
    if (m_bridge.isNull() || m_settings.isNull() || m_decoration) {
        return;
    }

    m_decoration = m_bridge->createDecoration(0);

    if (!m_decoration) {
        return;
    }

    m_decoration->setProperty("visualParent", QVariant::fromValue(this));
    m_client = m_bridge->lastCreatedClient();
    connect(m_decoration, &KDecoration2::Decoration::bordersChanged, this, &PreviewItem::syncSize);
    connect(m_decoration, &KDecoration2::Decoration::shadowChanged, this, &PreviewItem::syncSize);
    emit decorationChanged(m_decoration);
}

KDecoration2::Decoration *PreviewItem::decoration() const
{
    return m_decoration;
}

void PreviewItem::setDecoration(KDecoration2::Decoration *deco)
{
    if (m_decoration == deco) {
        return;
    }

    auto updateSlot = static_cast<void (QQuickItem::*)()>(&QQuickItem::update);

    if (m_decoration) {
        disconnect(m_decoration, &KDecoration2::Decoration::bordersChanged, this, updateSlot);
    }

    m_decoration = deco;
    m_decoration->setProperty("visualParent", QVariant::fromValue(this));
    connect(m_decoration, &KDecoration2::Decoration::bordersChanged, this, updateSlot);
    connect(m_decoration, &KDecoration2::Decoration::sectionUnderMouseChanged, this,
    [this](Qt::WindowFrameSection section) {
        switch (section) {
            case Qt::TopRightSection:
            case Qt::BottomLeftSection:
                setCursor(Qt::SizeBDiagCursor);
                return;

            case Qt::TopLeftSection:
            case Qt::BottomRightSection:
                setCursor(Qt::SizeFDiagCursor);
                return;

            case Qt::TopSection:
            case Qt::BottomSection:
                setCursor(Qt::SizeVerCursor);
                return;

            case Qt::LeftSection:
            case Qt::RightSection:
                setCursor(Qt::SizeHorCursor);
                return;

            default:
                setCursor(Qt::ArrowCursor);
        }
    }
           );
    connect(m_decoration, &KDecoration2::Decoration::shadowChanged, this, &PreviewItem::shadowChanged);
    emit decorationChanged(m_decoration);
}

QColor PreviewItem::windowColor() const
{
    return m_windowColor;
}

void PreviewItem::setWindowColor(const QColor &color)
{
    if (m_windowColor == color) {
        return;
    }

    m_windowColor = color;
    emit windowColorChanged(m_windowColor);
    update();
}

void PreviewItem::paint(QPainter *painter)
{
    if (!m_decoration) {
        return;
    }

    int paddingLeft   = 0;
    int paddingTop    = 0;
    int paddingRight  = 0;
    int paddingBottom = 0;

    m_decoration->paint(painter, QRect(0, 0, width(), height()));

    // if (m_drawBackground) {
    //    painter->fillRect(0, 0, width(), height(), m_windowColor);
    // }
}

void PreviewItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    QCoreApplication::instance()->sendEvent(decoration(), event);
}

void PreviewItem::mousePressEvent(QMouseEvent *event)
{
    QCoreApplication::instance()->sendEvent(decoration(), event);
}

void PreviewItem::mouseReleaseEvent(QMouseEvent *event)
{
    QCoreApplication::instance()->sendEvent(decoration(), event);
}

void PreviewItem::mouseMoveEvent(QMouseEvent *event)
{
    QCoreApplication::instance()->sendEvent(decoration(), event);
}

void PreviewItem::hoverEnterEvent(QHoverEvent *event)
{
    QCoreApplication::instance()->sendEvent(decoration(), event);
}

void PreviewItem::hoverLeaveEvent(QHoverEvent *event)
{
    QCoreApplication::instance()->sendEvent(decoration(), event);
}

void PreviewItem::hoverMoveEvent(QHoverEvent *event)
{
    QCoreApplication::instance()->sendEvent(decoration(), event);
}

bool PreviewItem::isDrawingBackground() const
{
    return m_drawBackground;
}

void PreviewItem::setDrawingBackground(bool set)
{
    if (m_drawBackground == set) {
        return;
    }

    m_drawBackground = set;
    emit drawingBackgroundChanged(set);
}

PreviewBridge *PreviewItem::bridge() const
{
    return m_bridge.data();
}

void PreviewItem::setBridge(PreviewBridge *bridge)
{
    if (m_bridge == bridge) {
        return;
    }

    if (m_bridge) {
        m_bridge->unregisterPreviewItem(this);
    }

    m_bridge = bridge;

    if (m_bridge) {
        m_bridge->registerPreviewItem(this);
    }

    emit bridgeChanged();
}

Settings *PreviewItem::settings() const
{
    return m_settings.data();
}

void PreviewItem::setSettings(Settings *settings)
{
    if (m_settings == settings) {
        return;
    }

    m_settings = settings;
    emit settingsChanged();
}

PreviewClient *PreviewItem::client()
{
    return m_client.data();
}

void PreviewItem::syncSize()
{
    if (!m_client) {
        return;
    }

    int widthOffset = 0;
    int heightOffset = 0;
    auto shadow = m_decoration->shadow();

    if (shadow) {
        widthOffset = shadow->paddingLeft() + shadow->paddingRight();
        heightOffset = shadow->paddingTop() + shadow->paddingBottom();
    }

    m_client->setWidth(width() - m_decoration->borderLeft() - m_decoration->borderRight() - widthOffset);
    m_client->setHeight(height() - m_decoration->borderTop() - m_decoration->borderBottom() - heightOffset);
}

KDecoration2::DecorationShadow *PreviewItem::shadow() const
{
    if (!m_decoration) {
        return nullptr;
    }

    const auto &s = m_decoration->shadow();

    if (!s) {
        return nullptr;
    }

    return s.data();
}

} // namespace Applet
} // namespace Decoration