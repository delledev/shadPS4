// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "game_grid_frame.h"

GameGridFrame::GameGridFrame(std::shared_ptr<GameInfoClass> game_info_get,
                             std::shared_ptr<GuiSettings> m_gui_settings, QWidget* parent)
    : QTableWidget(parent) {
    m_game_info = game_info_get;
    m_gui_settings_ = m_gui_settings;
    icon_size = m_gui_settings->GetValue(gui::m_icon_size_grid).toInt();
    windowWidth = parent->width();
    this->setShowGrid(false);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionBehavior(QAbstractItemView::SelectItems);
    this->setSelectionMode(QAbstractItemView::SingleSelection);
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->verticalScrollBar()->installEventFilter(this);
    this->verticalScrollBar()->setSingleStep(20);
    this->horizontalScrollBar()->setSingleStep(20);
    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    PopulateGameGrid(m_game_info->m_games, false);

    connect(this, &QTableWidget::cellClicked, this, &GameGridFrame::SetGridBackgroundImage);

    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this,
            &GameGridFrame::RefreshGridBackgroundImage);
    connect(this->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            &GameGridFrame::RefreshGridBackgroundImage);
    connect(this, &QTableWidget::customContextMenuRequested, this, [=, this](const QPoint& pos) {
        m_gui_context_menus.RequestGameMenu(pos, m_game_info->m_games, this, false);
    });
}

void GameGridFrame::PopulateGameGrid(QVector<GameInfo> m_games_search, bool fromSearch) {
    QVector<GameInfo> m_games_;
    this->clearContents();
    if (fromSearch)
        m_games_ = m_games_search;
    else
        m_games_ = m_game_info->m_games;
    m_games_shared = std::make_shared<QVector<GameInfo>>(m_games_);

    icon_size = m_gui_settings_->GetValue(gui::m_icon_size_grid)
                    .toInt(); // update icon size for resize event.

    int gamesPerRow = windowWidth / (icon_size + 20); // 2 x cell widget border size.
    int row = 0;
    int gameCounter = 0;
    int rowCount = m_games_.size() / gamesPerRow;
    if (m_games_.size() % gamesPerRow != 0) {
        rowCount += 1; // Add an extra row for the remainder
    }

    int column = 0;
    this->setColumnCount(gamesPerRow);
    this->setRowCount(rowCount);
    for (int i = 0; i < m_games_.size(); i++) {
        QWidget* widget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout();
        QLabel* image_label = new QLabel();
        QPixmap icon = m_games_[gameCounter].icon.scaled(
            QSize(icon_size, icon_size), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        image_label->setFixedSize(icon.width(), icon.height());
        image_label->setPixmap(icon);
        QLabel* name_label = new QLabel(QString::fromStdString(m_games_[gameCounter].serial));
        name_label->setAlignment(Qt::AlignHCenter);
        layout->addWidget(image_label);
        layout->addWidget(name_label);

        name_label->setStyleSheet("color: white; font-size: 12px; font-weight: bold;");
        QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
        shadowEffect->setBlurRadius(5);               // Set the blur radius of the shadow
        shadowEffect->setColor(QColor(0, 0, 0, 160)); // Set the color and opacity of the shadow
        shadowEffect->setOffset(2, 2);                // Set the offset of the shadow

        name_label->setGraphicsEffect(shadowEffect);
        widget->setLayout(layout);
        QString tooltipText = QString::fromStdString(m_games_[gameCounter].name);
        widget->setToolTip(tooltipText);
        QString tooltipStyle = QString("QToolTip {"
                                       "background-color: #ffffff;"
                                       "color: #000000;"
                                       "border: 1px solid #000000;"
                                       "padding: 2px;"
                                       "font-size: 12px; }")
                                   .arg(tooltipText);
        widget->setStyleSheet(tooltipStyle);
        this->setCellWidget(row, column, widget);

        column++;
        if (column == gamesPerRow) {
            column = 0;
            row++;
        }

        gameCounter++;
        if (gameCounter >= m_games_.size()) {
            break;
        }
    }
    m_games_.clear();
    this->resizeRowsToContents();
    this->resizeColumnsToContents();
}

void GameGridFrame::SetGridBackgroundImage(int row, int column) {

    int itemID = (row * this->columnCount()) + column;
    QWidget* item = this->cellWidget(row, column);
    if (item) {
        QString pic1Path = QString::fromStdString((*m_games_shared)[itemID].pic_path);
        QString blurredPic1Path =
            qApp->applicationDirPath() +
            QString::fromStdString("/game_data/" + (*m_games_shared)[itemID].serial + "/pic1.png");

        backgroundImage = QImage(blurredPic1Path);
        if (backgroundImage.isNull()) {
            QImage image(pic1Path);
            backgroundImage = m_game_list_utils.BlurImage(image, image.rect(), 16);

            std::filesystem::path img_path =
                std::filesystem::path("game_data/") / (*m_games_shared)[itemID].serial;
            std::filesystem::create_directories(img_path);
            if (!backgroundImage.save(blurredPic1Path, "PNG")) {
                // qDebug() << "Error: Unable to save image.";
            }
        }
        RefreshGridBackgroundImage();
    }
}

void GameGridFrame::RefreshGridBackgroundImage() {
    QPixmap blurredPixmap = QPixmap::fromImage(backgroundImage);
    QPalette palette;
    palette.setBrush(QPalette::Base, QBrush(blurredPixmap.scaled(size(), Qt::IgnoreAspectRatio)));
    QColor transparentColor = QColor(135, 206, 235, 40);
    palette.setColor(QPalette::Highlight, transparentColor);
    this->setPalette(palette);
}