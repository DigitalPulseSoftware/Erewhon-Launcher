// Copyright (C) 2017 Full Cycle Games
// This file is part of the "Load'n'Rush Server Console" project
// For conditions of distribution and use, see copyright notice in COPYRIGHT

#pragma once

#ifndef EREWHONLAUNCHER_DOWNLOADMANAGER_HPP
#define EREWHONLAUNCHER_DOWNLOADMANAGER_HPP

#include <QtCore/QFile>
#include <QtCore/QQueue>
#include <QtCore/QTime>
#include <QtCore/QUrl>
#include <QtWidgets/QWidget>

class DownloadManager
{
	public:
		DownloadManager();

		void Download(const QUrl& url, QIODevice* output);

	private:
		QFile m_output;
		QNetworkAccessManager m_manager;
		QQueue<QUrl> m_downloadQueue;
		QNetworkReply* m_currentDownload;
		QTime m_downloadTime;

};

#endif // EREWHONLAUNCHER_DOWNLOADMANAGER_HPP
