// Copyright (C) 2017 Full Cycle Games
// This file is part of the "Load'n'Rush Server Console" project
// For conditions of distribution and use, see copyright notice in COPYRIGHT

#pragma once

#ifndef EREWHONLAUNCHER_MAINWINDOW_HPP
#define EREWHONLAUNCHER_MAINWINDOW_HPP

#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QWidget>

class QFile;
class QLabel;
class QProgressBar;
class QPushButton;
class QTextEdit;

class MainWindow : public QWidget
{
	public:
		MainWindow();

		~MainWindow();

	private:
		void DownloadNewsFeed();
		void DownloadManifest();

		void OnFileDownloadError(const QString& fileName);
		void OnFileDownloadFinish(QFile* fileOutput);
		void OnManifestDownloaded(const QByteArray& buffer);
		void OnManifestDownloadError(QNetworkReply* download);
		void OnStartButtonPressed();
		void ProcessDownloadList();

		void UpdateProgressBar(qint64 bytesTotal, qint64 bytesReceived);

		QLabel* m_statusLabel;
		QNetworkAccessManager m_network;
		QNetworkReply* m_currentDownload;
		QProgressBar* m_progressBar;
		QPushButton* m_startButton;
		QTextEdit* m_newsWidget;
		qint64 m_downloadedSize;
		qint64 m_downloadTotalSize;
		std::size_t m_downloadCounter;
		std::vector<QString> m_downloadList;
		bool m_isUpdating;
};

#include <Launcher/Widgets/MainWindow.inl>

#endif // EREWHONLAUNCHER_MAINWINDOW_HPP
