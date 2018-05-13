// Copyright (C) 2017 Full Cycle Games
// This file is part of the "Load'n'Rush Server Console" project
// For conditions of distribution and use, see copyright notice in COPYRIGHT
/*
#include <Launcher/Widgets/MainWindow.hpp>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <iostream>

MainWindow::MainWindow()
{
	resize(1024, 768);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addSpacing(200);

	m_newsWidget = new QTextEdit;
	m_newsWidget->setReadOnly(true);
	m_newsWidget->setHtml("<b>Oh hi</b>");
	m_newsWidget->setMinimumSize(640, 480);
	//m_newsWidget->setContentsMargins(QMargins(50, 50, 50, 50));
	//m_newsWidget->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

	mainLayout->addWidget(m_newsWidget, 0, Qt::AlignCenter);

	mainLayout->addWidget(new QLabel("<b>Downloading...</b>"), 0, Qt::AlignHCenter);

	QHBoxLayout* bottomLayout = new QHBoxLayout;

	m_progressBar = new QProgressBar;
	bottomLayout->addWidget(m_progressBar);

	QPushButton* startButton = new QPushButton("Play");
	bottomLayout->addWidget(startButton);

	mainLayout->addLayout(bottomLayout);

	setLayout(mainLayout);
}

MainWindow::~MainWindow()
{
}

//#include <Application/Widgets/MainWindow.moc>
*/