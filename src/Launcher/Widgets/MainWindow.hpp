// Copyright (C) 2017 Full Cycle Games
// This file is part of the "Load'n'Rush Server Console" project
// For conditions of distribution and use, see copyright notice in COPYRIGHT

#pragma once

#ifndef EREWHONLAUNCHER_MAINWINDOW_HPP
#define EREWHONLAUNCHER_MAINWINDOW_HPP

#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QWidget>

class QLabel;
class QProgressBar;
class QTextEdit;

class MainWindow : public QWidget
{
	public:
		MainWindow();

		~MainWindow();

	private:
		void DownloadNewsFeed();
		void StartUpdateProcess();

		void OnFileDownloadError(const QString& fileName, QNetworkReply* download);
		void OnManifestDownloaded(const QByteArray& buffer);
		void OnManifestDownloadError(QNetworkReply* download);
		void ProcessDownloadList();

		QLabel* m_statusLabel;
		QNetworkAccessManager m_network;
		QProgressBar* m_progressBar;
		QTextEdit* m_newsWidget;
		std::size_t m_downloadCounter;
		std::vector<QString> m_downloadList;
};

#include <Launcher/Widgets/MainWindow.inl>

#endif // EREWHONLAUNCHER_MAINWINDOW_HPP
